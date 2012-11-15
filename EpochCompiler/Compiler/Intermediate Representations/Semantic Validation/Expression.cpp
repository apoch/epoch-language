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
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"

#include "Compiler/CompileErrors.h"

#include "Utility/StringPool.h"


using namespace IRSemantics;


//
// Construct and initialize an expression IR node
//
Expression::Expression()
	: InferredType(Metadata::EpochType_Error),
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
bool Expression::Validate(const Namespace& curnamespace) const
{
	Metadata::EpochTypeID mytype = GetEpochType(curnamespace);
	return (mytype != Metadata::EpochType_Error && mytype != Metadata::EpochType_Infer);
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
bool Expression::CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	Coalesce(curnamespace, activescope, errors);

	bool result = true;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(curnamespace, activescope, inreturnexpr, errors))
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
	Metadata::EpochTypeID WalkAtomsForType(const std::vector<ExpressionAtom*>& atoms, Namespace& curnamespace, size_t& index, Metadata::EpochTypeID lastknowntype, CompileErrors& errors)
	{
		Metadata::EpochTypeID ret = lastknowntype;

		while(index < atoms.size())
		{
			if(ret == Metadata::EpochType_Infer)
			{
				index = atoms.size();
				break;
			}

			const ExpressionAtomOperator* opatom = dynamic_cast<const ExpressionAtomOperator*>(atoms[index]);
			if(opatom)
			{
				if(opatom->IsMemberAccess())
				{
					Function* func = curnamespace.Functions.GetIR(opatom->GetIdentifier());
					InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
					func->TypeInference(curnamespace, context, errors);
					ret = func->GetReturnType(curnamespace);
					++index;
				}
				else if(opatom->IsOperatorUnary(curnamespace))
				{
					Metadata::EpochTypeID operandtype = WalkAtomsForType(atoms, curnamespace, ++index, ret, errors);
					if(operandtype == Metadata::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					Metadata::EpochTypeID underlyingtype = operandtype;
					if(Metadata::GetTypeFamily(operandtype) == Metadata::EpochTypeFamily_Unit)
						underlyingtype = curnamespace.Types.Aliases.GetStrongRepresentation(operandtype);

					ret = opatom->DetermineUnaryReturnType(curnamespace, underlyingtype, errors);
					if(underlyingtype != operandtype)
						ret = operandtype;
				}
				else
				{
					Metadata::EpochTypeID rhstype = WalkAtomsForType(atoms, curnamespace, ++index, ret, errors);
					if(rhstype == Metadata::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					Metadata::EpochTypeID originallhstype = ret;
					Metadata::EpochTypeID underlyingtypelhs = ret;
					if(Metadata::GetTypeFamily(ret) == Metadata::EpochTypeFamily_Unit)
						underlyingtypelhs = curnamespace.Types.Aliases.GetStrongRepresentation(ret);

					Metadata::EpochTypeID underlyingtyperhs = rhstype;
					if(Metadata::GetTypeFamily(rhstype) == Metadata::EpochTypeFamily_Unit)
						underlyingtyperhs = curnamespace.Types.Aliases.GetStrongRepresentation(rhstype);

					ret = opatom->DetermineOperatorReturnType(curnamespace, underlyingtypelhs, underlyingtyperhs, errors);
					if(underlyingtypelhs != originallhstype && ret == underlyingtypelhs)
						ret = originallhstype;
					else if(underlyingtyperhs != rhstype && ret == underlyingtyperhs)
						ret = rhstype;
				}

				break;
			}
			else
				ret = atoms[index++]->GetEpochType(curnamespace);
		}

		return ret;
	}


	//
	// Perform a similar traversal to the above, but with a different terminating
	// condition. This variant limits the number of atoms that will be examined,
	// making it easier to determine the type of specific subsections of a larger
	// expression.
	//
	Metadata::EpochTypeID WalkAtomsForTypePartial(const std::vector<ExpressionAtom*>& atoms, Namespace& curnamespace, size_t& index, Metadata::EpochTypeID lastknowntype, CompileErrors& errors)
	{
		Metadata::EpochTypeID ret = lastknowntype;

		while(index < atoms.size())
		{
			if(ret == Metadata::EpochType_Infer)
			{
				index = atoms.size();
				break;
			}

			const ExpressionAtomOperator* opatom = dynamic_cast<const ExpressionAtomOperator*>(atoms[index]);
			if(opatom)
			{
				if(opatom->IsMemberAccess())
				{
					Function* func = curnamespace.Functions.GetIR(opatom->GetIdentifier());
					InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
					func->TypeInference(curnamespace, context, errors);
					ret = func->GetReturnType(curnamespace);
					++index;
				}
				else if(opatom->IsOperatorUnary(curnamespace))
				{
					Metadata::EpochTypeID operandtype = WalkAtomsForType(atoms, curnamespace, ++index, ret, errors);
					if(operandtype == Metadata::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					Metadata::EpochTypeID underlyingtype = operandtype;
					if(Metadata::GetTypeFamily(operandtype) == Metadata::EpochTypeFamily_Unit)
						underlyingtype = curnamespace.Types.Aliases.GetStrongRepresentation(operandtype);

					ret = opatom->DetermineUnaryReturnType(curnamespace, underlyingtype, errors);
					if(underlyingtype != operandtype)
						ret = operandtype;
				}
				else
					break;
			}
			else
				ret = atoms[index++]->GetEpochType(curnamespace);
		}

		return ret;
	}

	//
	// Demote types to the smallest possible (and legal) matching type,
	// left-hand-side of operator version.
	//
	// This is used to handle the case where a literal is provided
	// that might fit into multiple primitive storage types, such as
	// the integer literal "42" (which could be 16, 32, 64 bits, etc.)
	//
	void DemoteLHS(std::vector<ExpressionAtom*>& atoms, Namespace& curnamespace, size_t& index, Metadata::EpochTypeID targettype)
	{
		while(index < atoms.size())
		{
			const ExpressionAtomOperator* opatom = dynamic_cast<const ExpressionAtomOperator*>(atoms[index]);
			if(opatom)
			{
				if(opatom->IsMemberAccess())
					++index;
				else if(opatom->IsOperatorUnary(curnamespace))
					++index;
				else
					break;
			}
			else
				atoms[index++]->Demote(targettype, curnamespace);
		}
	}

	//
	// Demote types to the smallest possible (and legal) matching type,
	// right-hand-side of operator version.
	//
	void DemoteRHS(std::vector<ExpressionAtom*>& atoms, Namespace& curnamespace, size_t& index, Metadata::EpochTypeID targettype)
	{
		while(index < atoms.size())
		{
			const ExpressionAtomOperator* opatom = dynamic_cast<const ExpressionAtomOperator*>(atoms[index]);
			if(opatom)
			{
				if(opatom->IsMemberAccess())
					++index;
				else if(opatom->IsOperatorUnary(curnamespace))
					++index;

				break;
			}
			else
				atoms[index++]->Demote(targettype, curnamespace);
		}
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
bool Expression::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors)
{
	// Compact certain operations - see Coalesce() for details
	Coalesce(curnamespace, activescope, errors);

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
		if(opatom && opatom->IsOperatorUnary(curnamespace) && !opatom->IsMemberAccess())
		{
			//
			// Perform type inference on unary operator atoms
			//
			// Once this is done, establish a new inference context for subsequent
			// atoms so that they will detect correctly that they are parameters to
			// the unary operator.
			//
			if(!(*iter)->TypeInference(curnamespace, activescope, newcontext, index, maxindex, errors))
				result = false;

			if(InferenceRecursed)
				return true;

			newcontext.ContextName = opatom->GetIdentifier();
			newcontext.ExpectedTypes.clear();
			newcontext.ExpectedTypes.push_back(curnamespace.Functions.GetExpectedTypes(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
			newcontext.ExpectedSignatures.clear();
			newcontext.ExpectedSignatures.push_back(curnamespace.Functions.GetExpectedSignatures(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
		}
		else if(opatom && !opatom->IsMemberAccess())
		{
			//
			// Perform type inference on general operators which are not structure member accesses
			//
			// Once this is done, establish a new inference context so that subsequent atoms
			// will correctly note that they are parameters to a binary operator.
			//
			if(!(*iter)->TypeInference(curnamespace, activescope, newcontext, index, maxindex, errors))
				result = false;

			if(InferenceRecursed)
				return true;

			std::vector<ExpressionAtom*>::iterator nextiter = iter;
			++nextiter;
			if(nextiter != Atoms.end())
			{
				newcontext.ContextName = opatom->GetIdentifier();
				newcontext.ExpectedTypes.clear();
				newcontext.ExpectedTypes.push_back(curnamespace.Functions.GetExpectedTypes(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
				newcontext.ExpectedSignatures.clear();
				newcontext.ExpectedSignatures.push_back(curnamespace.Functions.GetExpectedSignatures(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));

				if(!(*nextiter)->TypeInference(curnamespace, activescope, newcontext, 1, 2, errors))
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
			if(nextopatom && !nextopatom->IsMemberAccess() && !nextopatom->IsOperatorUnary(curnamespace))
			{
				InferenceContext atomcontext(nextopatom->GetIdentifier(), state);
				atomcontext.FunctionName = context.FunctionName;
				atomcontext.ExpectedTypes.push_back(curnamespace.Functions.GetExpectedTypes(nextopatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
				atomcontext.ExpectedSignatures.push_back(curnamespace.Functions.GetExpectedSignatures(nextopatom->GetIdentifier(), *activescope.GetScope(), context.ContextName, errors));
				if(!(*iter)->TypeInference(curnamespace, activescope, atomcontext, 0, 1, errors))
					result = false;

				if(InferenceRecursed)
					return true;
			}
		}
		else
		{
			if(!(*iter)->TypeInference(curnamespace, activescope, newcontext, index, maxindex, errors))
				result = false;

			if(InferenceRecursed)
				return true;
		}
	}

	//
	// Demote types as necessary
	//
	unsigned idx = 0;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
		if(!opatom || opatom->IsMemberAccess())
			continue;

		unsigned rhsidx = iter - Atoms.begin() + 1;

		unsigned originalidx = idx;
		unsigned originalrhsidx = rhsidx;

		Metadata::EpochTypeID typerhs = Metadata::EpochType_Error;
		typerhs = WalkAtomsForType(Atoms, curnamespace, rhsidx, typerhs, errors);

		Metadata::EpochTypeID underlyingtyperhs = typerhs;
		if(Metadata::GetTypeFamily(typerhs) == Metadata::EpochTypeFamily_Unit)
			underlyingtyperhs = curnamespace.Types.Aliases.GetStrongRepresentation(typerhs);

		bool hasoverloads = curnamespace.Functions.HasOverloads(opatom->GetIdentifier());
		if(hasoverloads)
		{
			const StringHandleSet& overloads = curnamespace.Functions.GetOverloadNames(opatom->GetIdentifier());
			for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
			{
				const FunctionSignature& overloadsig = curnamespace.Functions.GetSignature(*overloaditer);
				if(overloadsig.GetNumParameters() == 2)
				{
					idx = originalidx;

					bool lhswantsint16 = false;
					bool lhswantsint32 = false;
					bool rhswantsint16 = false;
					bool rhswantsint32 = false;
					bool matchlhs = false;
					bool matchrhs = false;

					Metadata::EpochTypeID typelhs = Metadata::EpochType_Error;
					typelhs = WalkAtomsForTypePartial(Atoms, curnamespace, idx, typelhs, errors);

					Metadata::EpochTypeID underlyingtypelhs = typelhs;
					if(Metadata::GetTypeFamily(typelhs) == Metadata::EpochTypeFamily_Unit)
						underlyingtypelhs = curnamespace.Types.Aliases.GetStrongRepresentation(typelhs);

					if(overloadsig.GetParameter(0).Type == Metadata::EpochType_Integer16)
						lhswantsint16 = true;
					else if(overloadsig.GetParameter(0).Type == Metadata::EpochType_Integer)
						lhswantsint32 = true;

					if(overloadsig.GetParameter(1).Type == Metadata::EpochType_Integer16)
						rhswantsint16 = true;
					else if(overloadsig.GetParameter(1).Type == Metadata::EpochType_Integer)
						rhswantsint32 = true;

					if(overloadsig.GetParameter(0).Type == typelhs || overloadsig.GetParameter(0).Type == underlyingtypelhs)
						matchlhs = true;

					if(overloadsig.GetParameter(1).Type == typerhs || overloadsig.GetParameter(1).Type == underlyingtyperhs)
						matchrhs = true;

					if(matchlhs && matchrhs)
						break;

					if(!matchlhs && matchrhs && lhswantsint16)
						DemoteLHS(Atoms, curnamespace, originalidx, Metadata::EpochType_Integer16);

					if(matchlhs && !matchrhs && rhswantsint16)
						DemoteRHS(Atoms, curnamespace, originalrhsidx, Metadata::EpochType_Integer16);
				}
			}
		}

		++idx;
	}

	//
	// Perform operator overload resolution
	//
	idx = 0;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
		if(!opatom || opatom->IsMemberAccess())
			continue;

		bool hasoverloads = curnamespace.Functions.HasOverloads(opatom->GetIdentifier());

		Metadata::EpochTypeID typerhs = Metadata::EpochType_Error;
		unsigned rhsidx = iter - Atoms.begin() + 1;
		typerhs = WalkAtomsForType(Atoms, curnamespace, rhsidx, typerhs, errors);

		Metadata::EpochTypeID underlyingtyperhs = typerhs;
		if(Metadata::GetTypeFamily(typerhs) == Metadata::EpochTypeFamily_Unit)
			underlyingtyperhs = curnamespace.Types.Aliases.GetStrongRepresentation(typerhs);

		if(curnamespace.Operators.PrefixExists(opatom->GetIdentifier()))
		{
			if(hasoverloads)
			{
				const StringHandleSet& overloads = curnamespace.Functions.GetOverloadNames(opatom->GetIdentifier());
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& overloadsig = curnamespace.Functions.GetSignature(*overloaditer);
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
			if(hasoverloads)
			{
				Metadata::EpochTypeID typelhs = Metadata::EpochType_Error;
				typelhs = WalkAtomsForTypePartial(Atoms, curnamespace, idx, typelhs, errors);

				Metadata::EpochTypeID underlyingtypelhs = typelhs;
				if(Metadata::GetTypeFamily(typelhs) == Metadata::EpochTypeFamily_Unit)
					underlyingtypelhs = curnamespace.Types.Aliases.GetStrongRepresentation(typelhs);

				bool found = false;
				const StringHandleSet& overloads = curnamespace.Functions.GetOverloadNames(opatom->GetIdentifier());
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& overloadsig = curnamespace.Functions.GetSignature(*overloaditer);
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
							if(opatom->DetermineOperatorReturnType(curnamespace, underlyingtypelhs, underlyingtyperhs, errors) == underlyingtypelhs)
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
	InferredType = Metadata::EpochType_Void;
	size_t i = 0;
	while(i < Atoms.size())
		InferredType = WalkAtomsForType(Atoms, curnamespace, i, InferredType, errors);

	result = (InferredType != Metadata::EpochType_Infer && InferredType != Metadata::EpochType_Error);

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
				if(opatom->IsOperatorUnary(curnamespace))
				{
					if(opatom->GetOperatorPrecedence(curnamespace) >= opatom2->GetOperatorPrecedence(curnamespace))
						break;
				}
				else
				{
					if(opatom->GetOperatorPrecedence(curnamespace) > opatom2->GetOperatorPrecedence(curnamespace))
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
Metadata::EpochTypeID Expression::GetEpochType(const Namespace&) const
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
void Expression::Coalesce(Namespace& curnamespace, CodeBlock& activescope, CompileErrors& errors)
{
	if(Coalesced)
		return;

	Coalesced = true;

	if(Atoms.empty())
		return;

	// Flatten member accesses
	Metadata::EpochTypeID structuretype = Metadata::EpochType_Error;
	bool completed;
	do
	{
		completed = true;
		for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
		{
			ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
			if(!opatom)
				continue;

			if(opatom->IsMemberAccess() && curnamespace.Strings.GetPooledString(opatom->GetIdentifier()) == L".")
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

					StringHandle structurename = curnamespace.Types.GetNameOfType(structuretype);

					ExpressionAtomIdentifier* opid = dynamic_cast<ExpressionAtomIdentifier*>(*nextiter);
					StringHandle memberaccessname = curnamespace.Functions.FindStructureMemberAccessOverload(structurename, opid->GetIdentifier());

					delete opatom;
					*iter = new ExpressionAtomOperator(memberaccessname, true);

					InferenceContext newcontext(0, InferenceContext::CONTEXT_GLOBAL);
					curnamespace.Functions.GetIR(memberaccessname)->TypeInference(curnamespace, newcontext, errors);
					structuretype = curnamespace.Functions.GetIR(memberaccessname)->GetReturnType(curnamespace);

					delete *nextiter;
					Atoms.erase(nextiter);
				}
				else
				{
					StringHandle structurename = curnamespace.Types.GetNameOfType(structuretype);

					ExpressionAtomIdentifier* opid = dynamic_cast<ExpressionAtomIdentifier*>(*nextiter);
					StringHandle memberaccessname = curnamespace.Functions.FindStructureMemberAccessOverload(structurename, opid->GetIdentifier());

					InferenceContext newcontext(0, InferenceContext::CONTEXT_GLOBAL);
					curnamespace.Functions.GetIR(memberaccessname)->TypeInference(curnamespace, newcontext, errors);
					structuretype = curnamespace.Functions.GetIR(memberaccessname)->GetReturnType(curnamespace);

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
Metadata::EpochTypeID ExpressionAtomStatement::GetEpochType(const Namespace& curnamespace) const
{
	return MyStatement->GetEpochType(curnamespace);
}

//
// Forward type inference requests to the wrapped statement
//
bool ExpressionAtomStatement::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t, CompileErrors& errors)
{
	return MyStatement->TypeInference(curnamespace, activescope, context, index, errors);
}

//
// Forward compile-time code execution requests to the wrapped statement
//
bool ExpressionAtomStatement::CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	return MyStatement->CompileTimeCodeExecution(curnamespace, activescope, inreturnexpr, errors);
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
Metadata::EpochTypeID ExpressionAtomParenthetical::GetEpochType(const Namespace& curnamespace) const
{
	if(MyParenthetical)
		return MyParenthetical->GetEpochType(curnamespace);

	return Metadata::EpochType_Error;
}

//
// Forward requests to perform type inference on parenthetical expressions
//
bool ExpressionAtomParenthetical::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t, size_t, CompileErrors& errors)
{
	return MyParenthetical->TypeInference(curnamespace, activescope, context, errors);
}

//
// Forward requests to perform compile-time code execution on parenthetical expressions
//
bool ExpressionAtomParenthetical::CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	return MyParenthetical->CompileTimeCodeExecution(curnamespace, activescope, inreturnexpr, errors);
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
Metadata::EpochTypeID ParentheticalPreOp::GetEpochType(const Namespace& curnamespace) const
{
	return MyStatement->GetEpochType(curnamespace);
}

//
// Forward type inference requests to a parenthetical pre-operation statement
//
bool ParentheticalPreOp::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const
{
	return MyStatement->TypeInference(curnamespace, activescope, context, errors);
}

//
// Pre-operations do not perform compile-time code execution
//
bool ParentheticalPreOp::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
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
Metadata::EpochTypeID ParentheticalPostOp::GetEpochType(const Namespace& curnamespace) const
{
	return MyStatement->GetEpochType(curnamespace);
}

//
// Forward type inference requests to a parenthetical post-operation statement
//
bool ParentheticalPostOp::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const
{
	return MyStatement->TypeInference(curnamespace, activescope, context, errors);
}

//
// Post-operations do not perform compile time code execution
//
bool ParentheticalPostOp::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
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
Metadata::EpochTypeID ParentheticalExpression::GetEpochType(const Namespace& curnamespace) const
{
	return MyExpression->GetEpochType(curnamespace);
}

//
// Forward type inference requests to a parenthetical expression
//
bool ParentheticalExpression::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) const
{
	return MyExpression->TypeInference(curnamespace, activescope, context, 0, 1, errors);
}

//
// Forward compile-time code execution requests to a parenthetical expression
//
bool ParentheticalExpression::CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	return MyExpression->CompileTimeCodeExecution(curnamespace, activescope, inreturnexpr, errors);
}


//
// Retrieve the effective type of an identifier atom
//
// Note that this requires the atom to have undergone type
// inference prior to the call, or results will be bogus.
//
Metadata::EpochTypeID ExpressionAtomIdentifierBase::GetEpochType(const Namespace&) const
{
	return MyType;
}

//
// Perform type inference on an identifier atom
//
bool ExpressionAtomIdentifierBase::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, size_t maxindex, CompileErrors& errors)
{
	// Early out to avoid duplicate inference work
	if(MyType != Metadata::EpochType_Error)
		return (MyType != Metadata::EpochType_Infer);

	bool foundidentifier = activescope.GetScope()->HasVariable(Identifier);
	StringHandle resolvedidentifier = Identifier;
	Metadata::EpochTypeID vartype = foundidentifier ? activescope.GetScope()->GetVariableTypeByID(Identifier) : Metadata::EpochType_Infer;

	if(std::wstring(OriginalIdentifier.begin(), OriginalIdentifier.end()) == L"nothing")
		vartype = Metadata::EpochType_Nothing;

	Metadata::EpochTypeID underlyingtype = vartype;

	if(foundidentifier && Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_Unit)
		underlyingtype = curnamespace.Types.Aliases.GetStrongRepresentation(vartype);
	
	std::set<Metadata::EpochTypeID> possibletypes;
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

			Metadata::EpochTypeID paramtype = types[i][index];
			if(paramtype == Metadata::EpochType_Function)
			{
				possibletypes.insert(Metadata::EpochType_Function);
				signaturematches += curnamespace.Functions.FindMatchingFunctions(Identifier, context.ExpectedSignatures.back()[i][index], context, errors, resolvedidentifier);
			}
			else if(paramtype == underlyingtype)
				possibletypes.insert(vartype);
			else if(paramtype == Metadata::EpochType_Identifier)
				possibletypes.insert(Metadata::EpochType_Identifier);
		}
	}

	if(possibletypes.size() != 1)
	{
		MyType = vartype;		// This will correctly handle structure types and fall back to Infer if all else fails
	}
	else
	{
		if(*possibletypes.begin() == Metadata::EpochType_Function)
		{
			if(signaturematches == 1)
			{
				Identifier = resolvedidentifier;
				MyType = Metadata::EpochType_Function;
			}
			else
				MyType = vartype;
		}
		else
			MyType = *possibletypes.begin();
	}

	bool success = (MyType != Metadata::EpochType_Infer && MyType != Metadata::EpochType_Error);
	if(!success)
	{
		errors.SetContext(OriginalIdentifier);
		if(!foundidentifier)
			errors.SemanticError("Unrecognized identifier");
		else if(possibletypes.empty())
			errors.SemanticError("Unrecognized type");
		else if(*possibletypes.begin() == Metadata::EpochType_Function)
			errors.SemanticError("No matching functions");
		else
			errors.SemanticError("Type mismatch");
	}
	return success;
}

//
// Identifier atoms do not need compile time code execution
//
bool ExpressionAtomIdentifierBase::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Deep copy stub
//
ExpressionAtom* ExpressionAtomIdentifierBase::Clone() const
{
	//
	// This is a failure of the semantic pass implementation.
	//
	// The ExpressionAtomIdentifierBase class is only used as a shared
	// base for actual identifier atoms of various natures, and should
	// not be instantiated directly. Attempting to clone one indicates
	// that one exists, which is bad. Ideally we'd trap this situation
	// earlier, but this is the most convenient and simple solution.
	//
	throw InternalException("Cannot clone this class; use one of its subclasses");
}

//
// Deep copy an atom which is an identifier
//
ExpressionAtom* ExpressionAtomIdentifier::Clone() const
{
	ExpressionAtomIdentifier* clone = new ExpressionAtomIdentifier(Identifier, OriginalIdentifier);
	clone->MyType = MyType;
	return clone;
}

//
// Deep copy an atom which is a reference to an identifier
//
ExpressionAtom* ExpressionAtomIdentifierReference::Clone() const
{
	ExpressionAtomIdentifierReference* clone = new ExpressionAtomIdentifierReference(Identifier, OriginalIdentifier);
	clone->MyType = MyType;
	return clone;
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
Metadata::EpochTypeID ExpressionAtomOperator::GetEpochType(const Namespace&) const
{
	return Metadata::EpochType_Error;
}

//
// Operator atoms do not perform their own type inference work
//
// It is the duty of the expression as a whole to perform type
// inference on its atoms, since it has more contextual information
// about the expression itself than any of the individual atoms
// could have.
//
bool ExpressionAtomOperator::TypeInference(Namespace&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
{
	return true;
}

//
// Operator atoms do not perform compile-time code execution
//
bool ExpressionAtomOperator::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Determine if an operator is unary
//
bool ExpressionAtomOperator::IsOperatorUnary(const Namespace& curnamespace) const
{
	if(curnamespace.Functions.Exists(Identifier))
		return (curnamespace.Functions.GetIR(Identifier)->GetNumParameters() == 1);

	if(curnamespace.Functions.HasOverloads(Identifier))
	{
		const StringHandleSet& overloadnames = curnamespace.Functions.GetOverloadNames(Identifier);
		for(StringHandleSet::const_iterator oviter = overloadnames.begin(); oviter != overloadnames.end(); ++oviter)
		{
			if(curnamespace.Functions.GetSignature(*oviter).GetNumParameters() == 1)
				return true;	
		}
	}

	if(curnamespace.Functions.SignatureExists(Identifier) && curnamespace.Functions.GetSignature(Identifier).GetNumParameters() == 1)
		return true;

	return false;
}

//
// Determine the return type of a binary operator,
// given the types of its left-hand and right-hand
// operands. Performs overload resolution in order
// to obtain the correct operator overload.
//
Metadata::EpochTypeID ExpressionAtomOperator::DetermineOperatorReturnType(Namespace& curnamespace, Metadata::EpochTypeID lhstype, Metadata::EpochTypeID rhstype, CompileErrors& errors) const
{
	if(OverriddenType != Metadata::EpochType_Error)
		return OverriddenType;

	if(curnamespace.Functions.Exists(Identifier))
	{
		Function* func = curnamespace.Functions.GetIR(Identifier);
		InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
		func->TypeInference(curnamespace, context, errors);
		return func->GetReturnType(curnamespace);
	}

	if(curnamespace.Functions.HasOverloads(Identifier))
	{
		const StringHandleSet& overloads = curnamespace.Functions.GetOverloadNames(Identifier);
		for(StringHandleSet::const_iterator oviter = overloads.begin(); oviter != overloads.end(); ++oviter)
		{
			const FunctionSignature& signature = curnamespace.Functions.GetSignature(*oviter);
			if(signature.GetNumParameters() == 2)
			{
				if(signature.GetParameter(0).Type == lhstype && signature.GetParameter(1).Type == rhstype)
					return signature.GetReturnType();
			}
		}
	}

	const FunctionSignature& signature = curnamespace.Functions.GetSignature(Identifier);
	if(signature.GetNumParameters() == 2)
	{
		if(signature.GetParameter(0).Type == lhstype && signature.GetParameter(1).Type == rhstype)
			return signature.GetReturnType();
	}

	return Metadata::EpochType_Error;
}

//
// Determine the return type of a unary operator,
// given the operand's type. Performs overload
// resolution to obtain the correct operator
// overload given the types involved.
//
Metadata::EpochTypeID ExpressionAtomOperator::DetermineUnaryReturnType(Namespace& curnamespace, Metadata::EpochTypeID operandtype, CompileErrors& errors) const
{
	if(OverriddenType != Metadata::EpochType_Error)
		return OverriddenType;

	if(curnamespace.Functions.Exists(Identifier))
	{
		Function* func = curnamespace.Functions.GetIR(Identifier);
		InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
		func->TypeInference(curnamespace, context, errors);
		return func->GetReturnType(curnamespace);
	}

	if(curnamespace.Functions.HasOverloads(Identifier))
	{
		const StringHandleSet& overloads = curnamespace.Functions.GetOverloadNames(Identifier);
		for(StringHandleSet::const_iterator oviter = overloads.begin(); oviter != overloads.end(); ++oviter)
		{
			const FunctionSignature& signature = curnamespace.Functions.GetSignature(*oviter);
			if(signature.GetNumParameters() == 1)
			{
				if(signature.GetParameter(0).Type == operandtype)
					return signature.GetReturnType();
			}
		}
	}

	const FunctionSignature& signature = curnamespace.Functions.GetSignature(Identifier);
	if(signature.GetNumParameters() == 1)
	{
		if(signature.GetParameter(0).Type == operandtype)
			return signature.GetReturnType();
	}

	return Metadata::EpochType_Error;
}

//
// Determine the precedence ordering rank of an operator
//
int ExpressionAtomOperator::GetOperatorPrecedence(const Namespace& curnamespace) const
{
	return curnamespace.Operators.GetPrecedence(OriginalIdentifier);
}

//
// Deep copy an atom which is an operator
//
ExpressionAtom* ExpressionAtomOperator::Clone() const
{
	ExpressionAtomOperator* clone = new ExpressionAtomOperator(Identifier, IsMemberAccessFlag);
	clone->OriginalIdentifier = OriginalIdentifier;
	clone->OverriddenType = OverriddenType;
	return clone;
}


//
// Literals may have one of several types, due to type
// demotion rules (e.g. 32 bit -> 16 bit integer)
//
// Assumes that type inference has been performed on the
// expression containing this atom, and demotion logic
// has been invoked.
//
Metadata::EpochTypeID ExpressionAtomLiteralInteger32::GetEpochType(const Namespace&) const
{
	return MyType;
}

//
// Potentially demote a literal if necessary
//
bool ExpressionAtomLiteralInteger32::TypeInference(Namespace& curnamespace, CodeBlock&, InferenceContext& context, size_t index, size_t maxindex, CompileErrors&)
{
	bool expected32bits = false;
	bool expected16bits = false;

	if(context.ExpectedTypes.empty())
		return true;

	for(size_t i = 0; i < context.ExpectedTypes.back().size(); ++i)
	{
		if(context.ExpectedTypes.back()[i].size() < maxindex)
			continue;

		Metadata::EpochTypeID expectedtype = context.ExpectedTypes.back()[i][index];
		if(Metadata::GetTypeFamily(expectedtype) == Metadata::EpochTypeFamily_Unit)
		{
			if(curnamespace.Types.Aliases.GetStrongRepresentation(expectedtype) == Metadata::EpochType_Integer)
				MyType = expectedtype;
		}
		else if(expectedtype == Metadata::EpochType_Integer)
			expected32bits = true;
		else if(expectedtype == Metadata::EpochType_Integer16)
			expected16bits = true;
	}

	if(expected16bits && !expected32bits)
		return Demote(Metadata::EpochType_Integer16, curnamespace);

	return true;
}

//
// Literal integers do not need compile-time code execution
//
bool ExpressionAtomLiteralInteger32::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
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
CompileTimeParameter ExpressionAtomLiteralInteger32::ConvertToCompileTimeParam(const Namespace&) const
{
	CompileTimeParameter ret(L"@@autoctp", Metadata::EpochType_Integer);
	ret.Payload.IntegerValue = Value;
	ret.HasPayload = true;
	return ret;
}


//
// Return the type of a real literal; subject to type
// demotion rules, similar to integral types.
//
// Assumes type inference and demotion has been applied.
//
Metadata::EpochTypeID ExpressionAtomLiteralReal32::GetEpochType(const Namespace&) const
{
	return MyType;
}

//
// Perform strong-typedef inference and potentially type demotion
//
bool ExpressionAtomLiteralReal32::TypeInference(Namespace& curnamespace, CodeBlock&, InferenceContext& context, size_t index, size_t, CompileErrors&)
{
	for(size_t i = 0; i < context.ExpectedTypes.back().size(); ++i)
	{
		Metadata::EpochTypeID expectedtype = context.ExpectedTypes.back()[i][index];
		if(Metadata::GetTypeFamily(expectedtype) == Metadata::EpochTypeFamily_Unit)
		{
			if(curnamespace.Types.Aliases.GetStrongRepresentation(expectedtype) == Metadata::EpochType_Real)
				MyType = expectedtype;
		}
	}

	return true;
}

//
// Real literals do not need compile-time code execution
//
bool ExpressionAtomLiteralReal32::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Convert a real literal to a compile-time parameter
//
// Predominantly used for pattern matching.
//
CompileTimeParameter ExpressionAtomLiteralReal32::ConvertToCompileTimeParam(const Namespace&) const
{
	CompileTimeParameter ret(L"@@autoctp", Metadata::EpochType_Real);
	ret.Payload.RealValue = Value;
	ret.HasPayload = true;
	return ret;
}



//
// Boolean literals always have the same type
//
Metadata::EpochTypeID ExpressionAtomLiteralBoolean::GetEpochType(const Namespace&) const
{
	return Metadata::EpochType_Boolean;
}

//
// Boolean literals do not need type inference
//
bool ExpressionAtomLiteralBoolean::TypeInference(Namespace&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
{
	return true;
}

//
// Boolean literals do not need compile-time code execution
//
bool ExpressionAtomLiteralBoolean::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Convert a boolean literal to a compile-time parameter
//
// Predominantly used for pattern-matching.
//
CompileTimeParameter ExpressionAtomLiteralBoolean::ConvertToCompileTimeParam(const Namespace&) const
{
	CompileTimeParameter ret(L"@@autoctp", Metadata::EpochType_Boolean);
	ret.Payload.BooleanValue = Value;
	ret.HasPayload = true;
	return ret;
}


//
// Literal strings always have the same type
//
Metadata::EpochTypeID ExpressionAtomLiteralString::GetEpochType(const Namespace&) const
{
	return Metadata::EpochType_String;
}

//
// Literal strings do not need type inference
//
bool ExpressionAtomLiteralString::TypeInference(Namespace&, CodeBlock&, InferenceContext&, size_t, size_t, CompileErrors&)
{
	return true;
}

//
// Literal strings do not need compile-time code execution
//
bool ExpressionAtomLiteralString::CompileTimeCodeExecution(Namespace&, CodeBlock&, bool, CompileErrors&)
{
	// No op
	return true;
}

//
// Convert a string literal to a compile-time parameter
//
// Predominantly used for pattern-matching.
//
CompileTimeParameter ExpressionAtomLiteralString::ConvertToCompileTimeParam(const Namespace& curnamespace) const
{
	CompileTimeParameter ret(L"@@autoctp", Metadata::EpochType_String);
	ret.Payload.LiteralStringHandleValue = Handle;
	ret.StringPayload = curnamespace.Strings.GetPooledString(Handle);
	ret.HasPayload = true;
	return ret;
}


//
// Deep copy an expression
//
Expression* Expression::Clone() const
{
	Expression* clone = new Expression;
	for(std::vector<ExpressionAtom*>::const_iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
		clone->Atoms.push_back((*iter)->Clone());

	clone->InferredType = InferredType;
	clone->Coalesced = Coalesced;
	clone->InferenceDone = false;
	clone->DoingInference = false;
	clone->InferenceRecursed = false;

	return clone;
}

//
// Deep copy an atom used to mark reference bindings
//
ExpressionAtom* ExpressionAtomBindReference::Clone() const
{
	return new ExpressionAtomBindReference(Identifier, MyType);
}

//
// Deep copy an atom representing a statement (function call)
//
ExpressionAtom* ExpressionAtomStatement::Clone() const
{
	return new ExpressionAtomStatement(MyStatement->Clone());
}

//
// Deep copy an atom containing a parenthetical expression
//
ExpressionAtom* ExpressionAtomParenthetical::Clone() const
{
	return new ExpressionAtomParenthetical(MyParenthetical->Clone());
}

//
// Deep copy a parenthetical pre-operation statement
//
Parenthetical* ParentheticalPreOp::Clone() const
{
	return new ParentheticalPreOp(MyStatement->Clone());
}

//
// Deep copy a parenthetical post-operation statement
//
Parenthetical* ParentheticalPostOp::Clone() const
{
	return new ParentheticalPostOp(MyStatement->Clone());
}

//
// Deep copy a general parenthetical expression
//
Parenthetical* ParentheticalExpression::Clone() const
{
	return new ParentheticalExpression(MyExpression->Clone());
}

//
// Deep copy an atom used to emit type annotation metadata
//
ExpressionAtom* ExpressionAtomTypeAnnotation::Clone() const
{
	return new ExpressionAtomTypeAnnotation(MyType);
}

//
// Deep copy a helper atom for reading structure member data
//
ExpressionAtom* ExpressionAtomCopyFromStructure::Clone() const
{
	return new ExpressionAtomCopyFromStructure(MyType, MemberName);
}

//
// Deep copy a literal string atom
//
ExpressionAtom* ExpressionAtomLiteralString::Clone() const
{
	ExpressionAtomLiteralString* clone = new ExpressionAtomLiteralString(Handle);
	return clone;
}

//
// Deep copy a literal integral atom
//
ExpressionAtom* ExpressionAtomLiteralInteger32::Clone() const
{
	ExpressionAtomLiteralInteger32* clone = new ExpressionAtomLiteralInteger32(Value);
	clone->MyType = MyType;
	return clone;
}

//
// Deep copy a literal real atom
//
ExpressionAtom* ExpressionAtomLiteralReal32::Clone() const
{
	ExpressionAtomLiteralReal32* clone = new ExpressionAtomLiteralReal32(Value);
	clone->MyType = MyType;
	return clone;
}

//
// Type demotion logic for integer literals
//
bool ExpressionAtomLiteralInteger32::Demote(Metadata::EpochTypeID target, const Namespace&)
{
	if(target != Metadata::EpochType_Integer16)
		return false;

	if(static_cast<UInteger32>(Value) > std::numeric_limits<UInteger16>::max())
		return false;

	MyType = target;
	return true;
}

