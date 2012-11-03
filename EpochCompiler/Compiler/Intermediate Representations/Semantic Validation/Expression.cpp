//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing expressions
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Session.h"

#include "User Interface/Output.h"


using namespace IRSemantics;


//
// Construct and initialize an expression IR node
//
Expression::Expression()
	: InferredType(VM::EpochType_Error),
	  Coalesced(false),
	  AtomsArePatternMatchedLiteral(false),
	  InferenceDone(false),
	  DoingInference(false),
	  InferenceRecursed(false)
{
}

//
// Destruct and clean up an expression IR node
//
Expression::~Expression()
{
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
		delete *iter;
}


//
// Perform semantic validation on an expression
//
bool Expression::Validate(const Program& program) const
{
	VM::EpochTypeID mytype = GetEpochType(program);
	return (mytype != VM::EpochType_Error && mytype != VM::EpochType_Infer);
}

//
// Perform compile-time code exeuction on an expression
//
// Primarily used to forward CTCE requests on to the atoms in
// the expression, with the intent of invoking CTCE for any
// statements that might be contained therein. This is generally
// to support the use of tagged function invocations within
// expressions.
//
bool Expression::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	Coalesce(program, activescope, errors);

	bool result = true;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, activescope, inreturnexpr, errors))
			result = false;
	}

	return result;
}

//
// Internal helpers
//
namespace
{
	//
	// Traverse a sequence of expression atoms and determine their types
	//
	// This traversal begins at the given index and walks rightwards in the expression
	// until it encounters an atom that represents a different "part" of that expression.
	// For example, in the expression "a.b = foo + c", the atoms "a" "." and "b" are
	// considered the same "part" since the . operator has maximum precedence and always
	// groups atoms on its left and right into a single unit.
	//
	// A traversal of this expression will break it apart into the sequences "a.b", "foo",
	// and "c". The = operator and the + operator are also considered, and given types
	// according to the types of their left and right hand sides. Unary operators have a
	// similar treatment, although their associativity is to the right instead of the left.
	//
	// This function is used to help determine the overall type of an expression as well
	// as to aid in type inference on subsections of the expression itself.
	//
	VM::EpochTypeID WalkAtomsForType(const std::vector<ExpressionAtom*>& atoms, Program& program, size_t& index, VM::EpochTypeID lastknowntype, CompileErrors& errors)
	{
		VM::EpochTypeID ret = lastknowntype;

		while(index < atoms.size())
		{
			if(ret == VM::EpochType_Infer)
			{
				index = atoms.size();
				break;
			}

			const ExpressionAtomOperator* opatom = dynamic_cast<const ExpressionAtomOperator*>(atoms[index]);
			if(opatom)
			{
				if(opatom->IsMemberAccess())
				{
					Function* func = program.GetFunctions().find(opatom->GetIdentifier())->second;
					InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
					func->TypeInference(program, context, errors);
					ret = func->GetReturnType(program);
					++index;
				}
				else if(opatom->IsOperatorUnary(program))
				{
					VM::EpochTypeID operandtype = WalkAtomsForType(atoms, program, ++index, ret, errors);
					if(operandtype == VM::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					VM::EpochTypeID underlyingtype = operandtype;
					if(VM::GetTypeFamily(operandtype) == VM::EpochTypeFamily_Unit)
					{
						if(program.StrongTypeAliasRepresentations.find(operandtype) != program.StrongTypeAliasRepresentations.end())
							underlyingtype = program.StrongTypeAliasRepresentations[operandtype];
					}

					ret = opatom->DetermineUnaryReturnType(program, underlyingtype, errors);
					if(underlyingtype != operandtype)
						ret = operandtype;
				}
				else
				{
					VM::EpochTypeID rhstype = WalkAtomsForType(atoms, program, ++index, ret, errors);
					if(rhstype == VM::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					VM::EpochTypeID originallhstype = ret;
					VM::EpochTypeID underlyingtypelhs = ret;
					if(VM::GetTypeFamily(ret) == VM::EpochTypeFamily_Unit)
					{
						if(program.StrongTypeAliasRepresentations.find(ret) != program.StrongTypeAliasRepresentations.end())
							underlyingtypelhs = program.StrongTypeAliasRepresentations[ret];
					}

					VM::EpochTypeID underlyingtyperhs = rhstype;
					if(VM::GetTypeFamily(rhstype) == VM::EpochTypeFamily_Unit)
					{
						if(program.StrongTypeAliasRepresentations.find(rhstype) != program.StrongTypeAliasRepresentations.end())
							underlyingtyperhs = program.StrongTypeAliasRepresentations[rhstype];
					}

					ret = opatom->DetermineOperatorReturnType(program, underlyingtypelhs, underlyingtyperhs, errors);
					if(underlyingtypelhs != originallhstype && ret == underlyingtypelhs)
						ret = originallhstype;
					else if(underlyingtyperhs != rhstype && ret == underlyingtyperhs)
						ret = rhstype;
				}

				break;
			}
			else
				ret = atoms[index++]->GetEpochType(program);
		}

		return ret;
	}


	//
	// Perform a similar traversal to the above, but with a different terminating
	// condition. This variant limits the number of atoms that will be examined,
	// making it easier to determine the type of specific subsections of a larger
	// expression.
	//
	VM::EpochTypeID WalkAtomsForTypePartial(const std::vector<ExpressionAtom*>& atoms, Program& program, size_t& index, VM::EpochTypeID lastknowntype, CompileErrors& errors)
	{
		VM::EpochTypeID ret = lastknowntype;

		while(index < atoms.size())
		{
			if(ret == VM::EpochType_Infer)
			{
				index = atoms.size();
				break;
			}

			const ExpressionAtomOperator* opatom = dynamic_cast<const ExpressionAtomOperator*>(atoms[index]);
			if(opatom)
			{
				if(opatom->IsMemberAccess())
				{
					Function* func = program.GetFunctions().find(opatom->GetIdentifier())->second;
					InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
					func->TypeInference(program, context, errors);
					ret = func->GetReturnType(program);
					++index;
				}
				else if(opatom->IsOperatorUnary(program))
				{
					VM::EpochTypeID operandtype = WalkAtomsForType(atoms, program, ++index, ret, errors);
					if(operandtype == VM::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					VM::EpochTypeID underlyingtype = operandtype;
					if(VM::GetTypeFamily(operandtype) == VM::EpochTypeFamily_Unit)
					{
						if(program.StrongTypeAliasRepresentations.find(operandtype) != program.StrongTypeAliasRepresentations.end())
							underlyingtype = program.StrongTypeAliasRepresentations[operandtype];
					}

					ret = opatom->DetermineUnaryReturnType(program, underlyingtype, errors);
					if(underlyingtype != operandtype)
						ret = operandtype;
				}
				else
					break;
			}
			else
				ret = atoms[index++]->GetEpochType(program);
		}

		return ret;
	}


	//
	// Helper structure for setting flags automatically via RAII idiom
	//
	// This is primarily used for detecting recursion in expression type
	// inference processes, which is important since without explicit
	// checks that recursion can easily become unbounded. Unfortunately,
	// we cannot simply check that a reasonable type has been inferred
	// as our recursion base condition, because inference might have
	// failed due to semantic errors in the program. Therefore we must
	// explicitly check for recursion and terminate it as quickly as
	// possible, using the last known result and assuming it is valid.
	//
	// In practice it does not appear that this recursion limitation
	// imposes any limits on the power of the type inference engine, but
	// that may need to be revisted as the language type system becomes
	// increasingly rich.
	//
	struct AutoFlag
	{
		AutoFlag(bool& flag, bool& recursed)
			: Flag(flag),
			  Recursed(recursed),
			  SetRecursion(false)
		{
			if(Flag)
				SetRecursion = true;

			Flag = true;
		}

		~AutoFlag()
		{
			Flag = false;
			if(SetRecursion)
				Recursed = true;
		}

	private:
		AutoFlag& operator= (const AutoFlag& rhs);

	private:
		bool& Flag;
		bool& Recursed;
		bool SetRecursion;
	};
}


//
// Perform type inference on an entire expression
//
bool Expression::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors)
{
	// Compact certain operations - see Coalesce() for details
	Coalesce(program, activescope, errors);

	// Early out so we don't waste time doing duplicate work
	if(InferenceDone)
		return true;

	// Set recursion detection up (see AutoFlag comments above)
	AutoFlag cleanup(DoingInference, InferenceRecursed);


	//
	// Establish a new context for type inference
	//
	// This context inherits the parent context's information.
	// The only difference is whether or not we pass along a new
	// "context state" to the expression's atoms. If the context
	// we are currently in is the return expression of a function,
	// we will pass along that state to the atoms, so that any
	// statements such as variable initializations within the
	// return expression will correctly detect that they are in
	// that special context. Otherwise, we simply inform the atoms
	// that they are in a generic expression context. Note that
	// we will mutate this context below during inference on large
	// non-trivial expressions.
	//
	InferenceContext::ContextStates state = context.State == InferenceContext::CONTEXT_FUNCTION_RETURN ? InferenceContext::CONTEXT_FUNCTION_RETURN : InferenceContext::CONTEXT_EXPRESSION;
	InferenceContext newcontext(context.ContextName, state);
	newcontext.FunctionName = context.FunctionName;
	newcontext.ExpectedTypes = context.ExpectedTypes;
	newcontext.ExpectedSignatures = context.ExpectedSignatures;

	bool result = true;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
		if(opatom && opatom->IsOperatorUnary(program) && !opatom->IsMemberAccess())
		{
			//
			// Perform type inference on unary operator atoms
			//
			// Once this is done, establish a new inference context for subsequent
			// atoms so that they will detect correctly that they are parameters to
			// the unary operator.
			//
			if(!(*iter)->TypeInference(program, activescope, newcontext, index, maxindex, errors))
				result = false;

			if(InferenceRecursed)
				return true;

			newcontext.ContextName = opatom->GetIdentifier();
			newcontext.ExpectedTypes.clear();
			newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
			newcontext.ExpectedSignatures.clear();
			newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
		}
		else if(opatom && !opatom->IsMemberAccess())
		{
			//
			// Perform type inference on general operators which are not structure member accesses
			//
			// Once this is done, establish a new inference context so that subsequent atoms
			// will correctly note that they are parameters to a binary operator.
			//
			if(!(*iter)->TypeInference(program, activescope, newcontext, index, maxindex, errors))
				result = false;

			if(InferenceRecursed)
				return true;

			std::vector<ExpressionAtom*>::iterator nextiter = iter;
			++nextiter;
			if(nextiter != Atoms.end())
			{
				newcontext.ContextName = opatom->GetIdentifier();
				newcontext.ExpectedTypes.clear();
				newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
				newcontext.ExpectedSignatures.clear();
				newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));

				if(!(*nextiter)->TypeInference(program, activescope, newcontext, 1, 2, errors))
					result = false;

				if(InferenceRecursed)
					return true;

				iter = nextiter;
			}

			continue;
		}

		//
		// Handle type inference for parameters to binary operators
		//
		std::vector<ExpressionAtom*>::iterator nextiter = iter;
		++nextiter;
		if(nextiter != Atoms.end())
		{
			ExpressionAtomOperator* nextopatom = dynamic_cast<ExpressionAtomOperator*>(*nextiter);
			if(nextopatom && !nextopatom->IsMemberAccess() && !nextopatom->IsOperatorUnary(program))
			{
				InferenceContext atomcontext(nextopatom->GetIdentifier(), state);
				atomcontext.FunctionName = context.FunctionName;
				atomcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(nextopatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
				atomcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(nextopatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
				if(!(*iter)->TypeInference(program, activescope, atomcontext, 0, 1, errors))
					result = false;

				if(InferenceRecursed)
					return true;
			}
		}
		else
		{
			if(!(*iter)->TypeInference(program, activescope, newcontext, index, maxindex, errors))
				result = false;

			if(InferenceRecursed)
				return true;
		}
	}

	//
	// Perform operator overload resolution
	//
	unsigned idx = 0;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
		if(!opatom || opatom->IsMemberAccess())
			continue;

		OverloadMap::const_iterator overloadmapiter = program.Session.FunctionOverloadNames.find(opatom->GetIdentifier());

		VM::EpochTypeID typerhs = VM::EpochType_Error;
		unsigned rhsidx = iter - Atoms.begin() + 1;
		typerhs = WalkAtomsForType(Atoms, program, rhsidx, typerhs, errors);

		VM::EpochTypeID underlyingtyperhs = typerhs;
		if(VM::GetTypeFamily(typerhs) == VM::EpochTypeFamily_Unit)
		{
			if(program.StrongTypeAliasRepresentations.find(typerhs) != program.StrongTypeAliasRepresentations.end())
				underlyingtyperhs = program.StrongTypeAliasRepresentations[typerhs];
		}

		if(program.Session.InfoTable.UnaryPrefixes->find(program.GetString(opatom->GetIdentifier())) != program.Session.InfoTable.UnaryPrefixes->end())
		{
			if(overloadmapiter != program.Session.FunctionOverloadNames.end())
			{
				const StringHandleSet& overloads = overloadmapiter->second;
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& overloadsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
					if(overloadsig.GetNumParameters() == 1)
					{
						if(overloadsig.GetParameter(0).Type == typerhs)
						{
							opatom->SetIdentifier(*overloaditer);
							break;
						}
					}
				}
			}
		}
		else
		{
			if(overloadmapiter != program.Session.FunctionOverloadNames.end())
			{
				VM::EpochTypeID typelhs = VM::EpochType_Error;
				typelhs = WalkAtomsForTypePartial(Atoms, program, idx, typelhs, errors);

				VM::EpochTypeID underlyingtypelhs = typelhs;
				if(VM::GetTypeFamily(typelhs) == VM::EpochTypeFamily_Unit)
				{
					if(program.StrongTypeAliasRepresentations.find(typelhs) != program.StrongTypeAliasRepresentations.end())
						underlyingtypelhs = program.StrongTypeAliasRepresentations[typelhs];
				}

				bool found = false;
				const StringHandleSet& overloads = overloadmapiter->second;
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& overloadsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
					if(overloadsig.GetNumParameters() == 2)
					{
						if(overloadsig.GetParameter(0).Type == typelhs && overloadsig.GetParameter(1).Type == typerhs)
						{
							opatom->SetIdentifier(*overloaditer);
							found = true;
							break;
						}
						else if(overloadsig.GetParameter(0).Type == underlyingtypelhs && overloadsig.GetParameter(1).Type == underlyingtyperhs)
						{
							opatom->SetIdentifier(*overloaditer);
							if(opatom->DetermineOperatorReturnType(program, underlyingtypelhs, underlyingtyperhs, errors) == underlyingtypelhs)
								opatom->OverrideType(typelhs);
							found = true;
							break;
						}
					}
				}

				if(!found)
					return false;

				++idx;
			}
		}
	}

	//
	// Abort if necessary
	//
	if(!result)
		return false;


	// Determine the type of the whole expression
	InferredType = VM::EpochType_Void;
	size_t i = 0;
	while(i < Atoms.size())
		InferredType = WalkAtomsForType(Atoms, program, i, InferredType, errors);

	result = (InferredType != VM::EpochType_Infer && InferredType != VM::EpochType_Error);

	// Perform operator precedence reordering via shunting yard method
	std::vector<ExpressionAtom*> outputqueue;
	std::vector<ExpressionAtomOperator*> opstack;
	for(size_t i = 0; i < Atoms.size(); ++i)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(Atoms[i]);
		if(opatom && !opatom->IsMemberAccess())
		{
			while(!opstack.empty())
			{
				ExpressionAtomOperator* opatom2 = opstack.back();
				if(opatom->IsOperatorUnary(program))
				{
					if(opatom->GetOperatorPrecedence(program) >= opatom2->GetOperatorPrecedence(program))
						break;
				}
				else
				{
					if(opatom->GetOperatorPrecedence(program) > opatom2->GetOperatorPrecedence(program))
						break;
				}

				outputqueue.push_back(opatom2);
				opstack.pop_back();
			}
			opstack.push_back(opatom);
		}
		else
		{
			outputqueue.push_back(Atoms[i]);
		}
	}
	while(!opstack.empty())
	{
		outputqueue.push_back(opstack.back());
		opstack.pop_back();
	}
	outputqueue.swap(Atoms);

	InferenceDone = true;
	if(!result)
		errors.SemanticError("Type error in operands");

	return result;
}

//
// Retrieve the type of an expression; note that this requires type inference
// to have been performed prior to the call, or erroneous values will result.
//
VM::EpochTypeID Expression::GetEpochType(const Program&) const
{
	return InferredType;
}


//
// Append an atom to the end of an expression
//
void Expression::AddAtom(ExpressionAtom* atom)
{
	Atoms.push_back(atom);
}


//
// Simplify certain sequences of operators and operands
//
// This is primarily used to turn sequences of the form "foo.bar.baz"
// (which breaks down into three operands and two operators) into a
// simpler form where the member access operator functions are located
// via overload resolution and invoked directly, resulting in a form
// with only one operand and two operator invocations; this can be
// thought of as "foo" followed by ".bar" followed by ".baz".
//
void Expression::Coalesce(Program& program, CodeBlock& activescope, CompileErrors& errors)
{
	if(Coalesced)
		return;

	Coalesced = true;

	if(Atoms.empty())
		return;

	// Flatten member accesses
	VM::EpochTypeID structuretype = VM::EpochType_Error;
	bool completed;
	do
	{
		completed = true;
		for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
		{
			ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
			if(!opatom)
				continue;

			if(program.GetString(opatom->GetIdentifier()) == L".")
			{
				std::vector<ExpressionAtom*>::iterator previter = iter;
				--previter;

				std::vector<ExpressionAtom*>::iterator nextiter = iter;
				++nextiter;

				ExpressionAtomIdentifier* identifieratom = dynamic_cast<ExpressionAtomIdentifier*>(*previter);
				if(identifieratom)
				{
					structuretype = activescope.GetScope()->GetVariableTypeByID(identifieratom->GetIdentifier());
					*previter = new ExpressionAtomIdentifierReference(identifieratom->GetIdentifier(), identifieratom->GetOriginalIdentifier());
					delete identifieratom;

					StringHandle structurename = program.GetNameOfStructureType(structuretype);

					ExpressionAtomIdentifier* opid = dynamic_cast<ExpressionAtomIdentifier*>(*nextiter);
					StringHandle memberaccessname = program.FindStructureMemberAccessOverload(structurename, opid->GetIdentifier());

					delete opatom;
					*iter = new ExpressionAtomOperator(memberaccessname, true);

					InferenceContext newcontext(0, InferenceContext::CONTEXT_GLOBAL);
					program.GetFunctions().find(memberaccessname)->second->TypeInference(program, newcontext, errors);
					structuretype = program.GetFunctions().find(memberaccessname)->second->GetReturnType(program);

					delete *nextiter;
					Atoms.erase(nextiter);
				}
				else
				{
					StringHandle structurename = program.GetNameOfStructureType(structuretype);

					ExpressionAtomIdentifier* opid = dynamic_cast<ExpressionAtomIdentifier*>(*nextiter);
					StringHandle memberaccessname = program.FindStructureMemberAccessOverload(structurename, opid->GetIdentifier());

					InferenceContext newcontext(0, InferenceContext::CONTEXT_GLOBAL);
					program.GetFunctions().find(memberaccessname)->second->TypeInference(program, newcontext, errors);
					structuretype = program.GetFunctions().find(memberaccessname)->second->GetReturnType(program);

					delete opatom;
					*iter = new ExpressionAtomBindReference(opid->GetIdentifier(), structuretype);

					delete *nextiter;
					Atoms.erase(nextiter);
				}

				completed = false;
				break;
			}
		}

	} while(!completed);
}



//
// Construct and initialize an expression atom which wraps a statement
//
ExpressionAtomStatement::ExpressionAtomStatement(Statement* statement)
	: MyStatement(statement)
{
}

//
// Destruct and clean up an expression atom which wraps a statement
//
ExpressionAtomStatement::~ExpressionAtomStatement()
{
	delete MyStatement;
}

//
// Retrieve the effective type of an expression atom wrapping a statement
//
VM::EpochTypeID ExpressionAtomStatement::GetEpochType(const Program& program) const
{
	return MyStatement->GetEpochType(program);
}

//
// Forward type inference requests to the wrapped statement
//
bool ExpressionAtomStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t, CompileErrors& errors)
{
	return MyStatement->TypeInference(program, activescope, context, index, errors);
}

//
// Forward compile-time code execution requests to the wrapped statement
//
bool ExpressionAtomStatement::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	return MyStatement->CompileTimeCodeExecution(program, activescope, inreturnexpr, errors);
}


//
// Construct and initialize a parenthetical expression atom
//
ExpressionAtomParenthetical::ExpressionAtomParenthetical(Parenthetical* parenthetical)
	: MyParenthetical(parenthetical)
{
}

//
// Destruct and clean up a parenthetical expression atom
//
ExpressionAtomParenthetical::~ExpressionAtomParenthetical()
{
	delete MyParenthetical;
}

//
// Retrieve the effective type of a parenthetical expression
//
VM::EpochTypeID ExpressionAtomParenthetical::GetEpochType(const Program& program) const
{
	if(MyParenthetical)
		return MyParenthetical->GetEpochType(program);

	return VM::EpochType_Error;
}

//
// Forward requests to perform type inference on parenthetical expressions
//
bool ExpressionAtomParenthetical::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t, size_t, CompileErrors& errors)
{
	return MyParenthetical->TypeInference(program, activescope, context, errors);
}

//
// Forward requests to perform compile-time code execution on parenthetical expressions
//
bool ExpressionAtomParenthetical::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	return MyParenthetical->CompileTimeCodeExecution(program, activescope, inreturnexpr, errors);
}


//
// Construct and initialize a parenthetical pre-operation statement
//
ParentheticalPreOp::ParentheticalPreOp(PreOpStatement* statement)
	: MyStatement(statement)
{
}

//
// Destruct and clean up a parenthetical pre-operation statement
//
ParentheticalPreOp::~ParentheticalPreOp()
{
	delete MyStatement;
}

//
// Retrieve the effective type of a parenthetical pre-operation statement
//
VM::EpochTypeID ParentheticalPreOp::GetEpochType(const Program& program) const
{
	return MyStatement->GetEpochType(program);
}

//
// Forward type inference requests to a parenthetical pre-operation statement
//
bool ParentheticalPreOp::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const
{
	return MyStatement->TypeInference(program, activescope, context, errors);
}

//
// Pre-operations do not perform compile-time code execution
//
bool ParentheticalPreOp::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	return true;
}


//
// Construct and initialize a parenthetical post-operation statement
//
ParentheticalPostOp::ParentheticalPostOp(PostOpStatement* statement)
	: MyStatement(statement)
{
}

//
// Destruct and clean up a parenthetical post-operation statement
//
ParentheticalPostOp::~ParentheticalPostOp()
{
	delete MyStatement;
}

//
// Retrieve the effective type of a parenthetical post-operation statement
//
VM::EpochTypeID ParentheticalPostOp::GetEpochType(const Program& program) const
{
	return MyStatement->GetEpochType(program);
}

//
// Forward type inference requests to a parenthetical post-operation statement
//
bool ParentheticalPostOp::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const
{
	return MyStatement->TypeInference(program, activescope, context, errors);
}

//
// Post-operations do not perform compile time code execution
//
bool ParentheticalPostOp::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	return true;
}


//
// Construct and initialize a wrapper for a parenthetical expression
//
ParentheticalExpression::ParentheticalExpression(Expression* expression)
	: MyExpression(expression)
{
}

//
// Destruct and clean up a wrapper for a parenthetical expression
//
ParentheticalExpression::~ParentheticalExpression()
{
	delete MyExpression;
}

//
// Retrieve the effective type of a parenthetical expression
//
VM::EpochTypeID ParentheticalExpression::GetEpochType(const Program& program) const
{
	return MyExpression->GetEpochType(program);
}

//
// Forward type inference requests to a parenthetical expression
//
bool ParentheticalExpression::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const
{
	return MyExpression->TypeInference(program, activescope, context, 0, 1, errors);
}

//
// Forward compile-time code execution requests to a parenthetical expression
//
bool ParentheticalExpression::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	return MyExpression->CompileTimeCodeExecution(program, activescope, inreturnexpr, errors);
}


//
// Retrieve the effective type of an identifier atom
//
// Note that this requires the atom to have undergone type
// inference prior to the call, or results will be bogus.
//
VM::EpochTypeID ExpressionAtomIdentifierBase::GetEpochType(const Program&) const
{
	return MyType;
}

//
// Perform type inference on an identifier atom
//
bool ExpressionAtomIdentifierBase::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors)
{
	// Early out to avoid duplicate inference work
	if(MyType != VM::EpochType_Error)
		return (MyType != VM::EpochType_Infer);

	bool foundidentifier = activescope.GetScope()->HasVariable(Identifier);
	StringHandle resolvedidentifier = Identifier;
	VM::EpochTypeID vartype = foundidentifier ? activescope.GetScope()->GetVariableTypeByID(Identifier) : VM::EpochType_Infer;

	if(program.GetString(Identifier) == L"nothing")
		vartype = VM::EpochType_Nothing;

	VM::EpochTypeID underlyingtype = vartype;

	if(foundidentifier && VM::GetTypeFamily(vartype) == VM::EpochTypeFamily_Unit)
	{
		if(program.StrongTypeAliasRepresentations.find(vartype) != program.StrongTypeAliasRepresentations.end())
			underlyingtype = program.StrongTypeAliasRepresentations[vartype];
	}
	
	std::set<VM::EpochTypeID> possibletypes;
	unsigned signaturematches = 0;

	if(!context.ExpectedTypes.empty())
	{
		const InferenceContext::PossibleParameterTypes& types = context.ExpectedTypes.back();
		for(size_t i = 0; i < types.size(); ++i)
		{
			if(types[i].size() <= index)
				continue;

			if(types[i].size() != maxindex)
				continue;

			VM::EpochTypeID paramtype = types[i][index];
			if(paramtype == VM::EpochType_Function)
			{
				possibletypes.insert(VM::EpochType_Function);
				signaturematches += program.FindMatchingFunctions(Identifier, context.ExpectedSignatures.back()[i][index], context, errors, resolvedidentifier);
			}
			else if(paramtype == underlyingtype)
				possibletypes.insert(vartype);
			else if(paramtype == VM::EpochType_Identifier)
				possibletypes.insert(VM::EpochType_Identifier);
		}
	}

	if(possibletypes.size() != 1)
	{
		MyType = vartype;		// This will correctly handle structure types and fall back to Infer if all else fails
	}
	else
	{
		if(*possibletypes.begin() == VM::EpochType_Function)
		{
			if(signaturematches == 1)
			{
				Identifier = resolvedidentifier;
				MyType = VM::EpochType_Function;
			}
			else
				MyType = vartype;
		}
		else
			MyType = *possibletypes.begin();
	}

	bool success = (MyType != VM::EpochType_Infer && MyType != VM::EpochType_Error);
	if(!success)
	{
		errors.SetContext(OriginalIdentifier);
		if(!foundidentifier)
			errors.SemanticError("Unrecognized identifier");
		else if(possibletypes.empty())
			errors.SemanticError("Unrecognized type");
		else if(*possibletypes.begin() == VM::EpochType_Function)
			errors.SemanticError("No matching functions");
		else
			errors.SemanticError("Type mismatch");
	}
	return success;
}

//
// Identifier atoms do not need compile time code execution
//
bool ExpressionAtomIdentifierBase::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Operator atoms do not inherently carry a type
//
// Note that we do treat operator atoms as typed in some situations,
// but only after overload resolution has been performed and type
// inference has told us the types of the operands. Since that logic
// is not encapsulated by the operator atom itself, we always return
// an error here.
//
VM::EpochTypeID ExpressionAtomOperator::GetEpochType(const Program&) const
{
	return VM::EpochType_Error;
}

//
// Operator atoms do not perform their own type inference work
//
// It is the duty of the expression as a whole to perform type
// inference on its atoms, since it has more contextual information
// about the expression itself than any of the individual atoms
// could have.
//
bool ExpressionAtomOperator::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
{
	return true;
}

//
// Operator atoms do not perform compile-time code execution
//
bool ExpressionAtomOperator::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Determine if an operator is unary
//
bool ExpressionAtomOperator::IsOperatorUnary(const Program& program) const
{
	if(program.HasFunction(Identifier))
		return (program.GetFunctions().find(Identifier)->second->GetNumParameters() == 1);

	OverloadMap::const_iterator ovmapiter = program.Session.FunctionOverloadNames.find(Identifier);
	if(ovmapiter != program.Session.FunctionOverloadNames.end())
	{
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(*oviter);
			if(funcsigiter != program.Session.FunctionSignatures.end())
			{
				if(funcsigiter->second.GetNumParameters() == 1)
					return true;	
			}
		}
	}

	FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(Identifier);
	if(funcsigiter != program.Session.FunctionSignatures.end())
	{
		if(funcsigiter->second.GetNumParameters() == 1)
			return true;
	}

	return false;
}

//
// Determine the return type of a binary operator,
// given the types of its left-hand and right-hand
// operands. Performs overload resolution in order
// to obtain the correct operator overload.
//
VM::EpochTypeID ExpressionAtomOperator::DetermineOperatorReturnType(Program& program, VM::EpochTypeID lhstype, VM::EpochTypeID rhstype, CompileErrors& errors) const
{
	if(OverriddenType != VM::EpochType_Error)
		return OverriddenType;

	if(program.HasFunction(Identifier))
	{
		Function* func = program.GetFunctions().find(Identifier)->second;
		InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
		func->TypeInference(program, context, errors);
		return func->GetReturnType(program);
	}

	OverloadMap::const_iterator ovmapiter = program.Session.FunctionOverloadNames.find(Identifier);
	if(ovmapiter != program.Session.FunctionOverloadNames.end())
	{
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(*oviter);
			if(funcsigiter != program.Session.FunctionSignatures.end())
			{
				if(funcsigiter->second.GetNumParameters() == 2)
				{
					if(funcsigiter->second.GetParameter(0).Type == lhstype && funcsigiter->second.GetParameter(1).Type == rhstype)
						return funcsigiter->second.GetReturnType();
				}
			}
		}
	}

	FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(Identifier);
	if(funcsigiter != program.Session.FunctionSignatures.end())
	{
		if(funcsigiter->second.GetNumParameters() == 2)
		{
			if(funcsigiter->second.GetParameter(0).Type == lhstype && funcsigiter->second.GetParameter(1).Type == rhstype)
				return funcsigiter->second.GetReturnType();
		}
	}

	return VM::EpochType_Error;
}

//
// Determine the return type of a unary operator,
// given the operand's type. Performs overload
// resolution to obtain the correct operator
// overload given the types involved.
//
VM::EpochTypeID ExpressionAtomOperator::DetermineUnaryReturnType(Program& program, VM::EpochTypeID operandtype, CompileErrors& errors) const
{
	if(OverriddenType != VM::EpochType_Error)
		return OverriddenType;

	if(program.HasFunction(Identifier))
	{
		Function* func = program.GetFunctions().find(Identifier)->second;
		InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
		func->TypeInference(program, context, errors);
		return func->GetReturnType(program);
	}

	OverloadMap::const_iterator ovmapiter = program.Session.FunctionOverloadNames.find(Identifier);
	if(ovmapiter != program.Session.FunctionOverloadNames.end())
	{
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(*oviter);
			if(funcsigiter != program.Session.FunctionSignatures.end())
			{
				if(funcsigiter->second.GetNumParameters() == 1)
				{
					if(funcsigiter->second.GetParameter(0).Type == operandtype)
						return funcsigiter->second.GetReturnType();
				}
			}
		}
	}

	FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(Identifier);
	if(funcsigiter != program.Session.FunctionSignatures.end())
	{
		if(funcsigiter->second.GetNumParameters() == 1)
		{
			if(funcsigiter->second.GetParameter(0).Type == operandtype)
				return funcsigiter->second.GetReturnType();
		}
	}

	return VM::EpochType_Error;
}

//
// Determine the precedence ordering rank of an operator
//
int ExpressionAtomOperator::GetOperatorPrecedence(const Program& program) const
{
	return program.Session.OperatorPrecedences.find(OriginalIdentifier)->second;
}


// TODO - documentation is a lie now that we do strong typedefs on primitives
//
// Literal integers always have the same type
//
VM::EpochTypeID ExpressionAtomLiteralInteger32::GetEpochType(const Program&) const
{
	return MyType;
}

//
// Literal integers do not need type inference
//
bool ExpressionAtomLiteralInteger32::TypeInference(Program& program, CodeBlock&, InferenceContext& context, size_t index, size_t maxindex, CompileErrors&)
{
	if(context.ExpectedTypes.empty())
		return true;

	for(size_t i = 0; i < context.ExpectedTypes.back().size(); ++i)
	{
		if(context.ExpectedTypes.back()[i].size() < maxindex)
			continue;

		VM::EpochTypeID expectedtype = context.ExpectedTypes.back()[i][index];
		if(VM::GetTypeFamily(expectedtype) == VM::EpochTypeFamily_Unit)
		{
			if(program.StrongTypeAliasRepresentations.find(expectedtype)->second == VM::EpochType_Integer)
				MyType = expectedtype;
		}
	}

	return true;
}

//
// Literal integers do not need compile-time code execution
//
bool ExpressionAtomLiteralInteger32::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Convert a literal integer to a compile-time parameter
//
// This is predominantly used for pattern-matching with
// statically known integer argument values for functions.
//
CompileTimeParameter ExpressionAtomLiteralInteger32::ConvertToCompileTimeParam(const Program&) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_Integer);
	ret.Payload.IntegerValue = Value;
	ret.HasPayload = true;
	return ret;
}


//
// Real literals always have the same type
//
VM::EpochTypeID ExpressionAtomLiteralReal32::GetEpochType(const Program&) const
{
	return MyType;
}

//
// Real literals do not need type inference
//
bool ExpressionAtomLiteralReal32::TypeInference(Program& program, CodeBlock&, InferenceContext& context, size_t index, size_t, CompileErrors&)
{
	for(size_t i = 0; i < context.ExpectedTypes.back().size(); ++i)
	{
		VM::EpochTypeID expectedtype = context.ExpectedTypes.back()[i][index];
		if(VM::GetTypeFamily(expectedtype) == VM::EpochTypeFamily_Unit)
		{
			if(program.StrongTypeAliasRepresentations.find(expectedtype)->second == VM::EpochType_Real)
				MyType = expectedtype;
		}
	}

	return true;
}

//
// Real literals do not need compile-time code execution
//
bool ExpressionAtomLiteralReal32::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Convert a real literal to a compile-time parameter
//
// Predominantly used for pattern matching.
//
CompileTimeParameter ExpressionAtomLiteralReal32::ConvertToCompileTimeParam(const Program&) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_Real);
	ret.Payload.RealValue = Value;
	ret.HasPayload = true;
	return ret;
}



//
// Boolean literals always have the same type
//
VM::EpochTypeID ExpressionAtomLiteralBoolean::GetEpochType(const Program&) const
{
	return VM::EpochType_Boolean;
}

//
// Boolean literals do not need type inference
//
bool ExpressionAtomLiteralBoolean::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
{
	return true;
}

//
// Boolean literals do not need compile-time code execution
//
bool ExpressionAtomLiteralBoolean::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Convert a boolean literal to a compile-time parameter
//
// Predominantly used for pattern-matching.
//
CompileTimeParameter ExpressionAtomLiteralBoolean::ConvertToCompileTimeParam(const Program&) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_Boolean);
	ret.Payload.BooleanValue = Value;
	ret.HasPayload = true;
	return ret;
}


//
// Literal strings always have the same type
//
VM::EpochTypeID ExpressionAtomLiteralString::GetEpochType(const Program&) const
{
	return VM::EpochType_String;
}

//
// Literal strings do not need type inference
//
bool ExpressionAtomLiteralString::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
{
	return true;
}

//
// Literal strings do not need compile-time code execution
//
bool ExpressionAtomLiteralString::CompileTimeCodeExecution(Program&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Convert a string literal to a compile-time parameter
//
// Predominantly used for pattern-matching.
//
CompileTimeParameter ExpressionAtomLiteralString::ConvertToCompileTimeParam(const Program& program) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_String);
	ret.Payload.LiteralStringHandleValue = Handle;
	ret.StringPayload = program.GetString(Handle);
	ret.HasPayload = true;
	return ret;
}

