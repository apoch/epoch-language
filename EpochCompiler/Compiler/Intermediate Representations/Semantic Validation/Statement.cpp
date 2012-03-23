//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class for representing statements
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Session.h"
#include "Compiler/Exceptions.h"


using namespace IRSemantics;


Statement::Statement(StringHandle name)
	: Name(name),
	  MyType(VM::EpochType_Infer)
{
}

Statement::~Statement()
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete *iter;
}

void Statement::AddParameter(Expression* expression)
{
	Parameters.push_back(expression);
}


bool Statement::Validate(const Program& program) const
{
	bool valid = true;

	for(std::vector<Expression*>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->Validate(program))
			valid = false;
	}

	return valid && (MyType != VM::EpochType_Infer) && (MyType != VM::EpochType_Error);
}

bool Statement::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, activescope, inreturnexpr))
			return false;
	}

	FunctionCompileHelperTable::const_iterator fchiter = program.Session.InfoTable.FunctionHelpers->find(Name);
	if(fchiter != program.Session.InfoTable.FunctionHelpers->end())
		fchiter->second(*this, program, activescope, inreturnexpr);

	return true;
}

bool Statement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	InferenceContext newcontext(Name, InferenceContext::CONTEXT_STATEMENT);
	newcontext.FunctionName = context.FunctionName;

	switch(context.State)
	{
	case InferenceContext::CONTEXT_CODE_BLOCK:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(Name, *activescope.GetScope(), context.FunctionName));
		newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(Name, *activescope.GetScope(), context.FunctionName));
		break;

	case InferenceContext::CONTEXT_STATEMENT:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(context.ContextName, *activescope.GetScope(), context.FunctionName));
		newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(context.ContextName, *activescope.GetScope(), context.FunctionName));
		break;

	case InferenceContext::CONTEXT_EXPRESSION:
	case InferenceContext::CONTEXT_FUNCTION_RETURN:
		// TODO - this is broken, evaluate the actual operators involved and use them w/ overload resolution
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(Name, *activescope.GetScope(), context.FunctionName));
		newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(Name, *activescope.GetScope(), context.FunctionName));
		break;

	default:
		//
		// This is a parser failure or the result of incomplete
		// language feature implementation. The statement node
		// in the AST has occurred in a context in which type
		// inference cannot be performed because the context is
		// not recognized by one of the above switch cases.
		//
		// Check to ensure the statement node is placed in a
		// valid location in the AST, that AST traversal is not
		// broken in some way, and that any new/partially done
		// language features are handled appropriately above.
		//
		throw InternalException("Statement type inference failure - unrecognized context");
	}

	size_t i = 0;
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, newcontext, i))
			return false;

		++i;
	}

	if(context.State == InferenceContext::CONTEXT_FUNCTION_RETURN)
		MyType = program.LookupType(Name);
	else
	{
		if(program.HasFunction(Name))
		{
			InferenceContext funccontext(0, InferenceContext::CONTEXT_GLOBAL);
			unsigned overloadcount = program.GetNumFunctionOverloads(Name);
			for(unsigned i = 0; i < overloadcount; ++i)
			{
				StringHandle overloadname = program.GetFunctionOverloadName(Name, i);
				Function* func = program.GetFunctions().find(overloadname)->second;
				func->TypeInference(program, funccontext);

				if(!context.ExpectedTypes.empty())
				{
					bool matchedreturn = false;
					const InferenceContext::PossibleParameterTypes& possibles = context.ExpectedTypes.back();
					for(size_t j = 0; j < possibles.size(); ++j)
					{
						if(possibles[j][index] == func->GetReturnType(program))
						{
							matchedreturn = true;
							break;
						}
					}

					if(!matchedreturn)
						continue;
				}
				
				if(func->GetNumParameters() == Parameters.size())
				{
					bool match = true;
					for(size_t j = 0; j < Parameters.size(); ++j)
					{
						VM::EpochTypeID thisparamtype = func->GetParameterTypeByIndex(j, program);
						if(thisparamtype != Parameters[j]->GetEpochType(program))
						{
							match = false;
							break;
						}

						// Validate function signatures
						if(thisparamtype == VM::EpochType_Function)
						{
							const ExpressionAtomIdentifier* identifieratom = dynamic_cast<const ExpressionAtomIdentifier*>(Parameters[j]->GetAtoms()[0]);
							StringHandle identifier = identifieratom->GetIdentifier();

							if(program.HasFunction(identifier))
							{
								// TODO
							}
							else
							{
								const FunctionSignature& funcsig = program.Session.FunctionSignatures.find(identifier)->second;
								if(!func->DoesParameterSignatureMatch(j, funcsig, program))
								{
									match = false;
									break;
								}
							}
						}
					}
					
					if(match)
					{
						Name = overloadname;
						MyType = func->GetReturnType(program);

						for(size_t j = 0; j < Parameters.size(); ++j)
						{
							if(func->IsParameterReference(func->GetParameterNames()[j]))
							{
								// TODO - bulletproof this a bit
								std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[j]->GetAtoms()[0]));
								Parameters[j]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier());
								Parameters[j]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, j);
							}
						}

						break;
					}
				}
			}
		}
		else
		{
			OverloadMap::const_iterator overloadmapiter = program.Session.FunctionOverloadNames.find(Name);
			if(overloadmapiter != program.Session.FunctionOverloadNames.end())
			{
				const StringHandleSet& overloads = overloadmapiter->second;
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& funcsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
					if(funcsig.GetNumParameters() == Parameters.size())
					{
						bool match = true;
						for(size_t i = 0; i < Parameters.size(); ++i)
						{
							if(funcsig.GetParameter(i).Type != Parameters[i]->GetEpochType(program))
							{
								match = false;
								break;
							}
						}

						if(match)
						{
							Name = *overloaditer;
							MyType = funcsig.GetReturnType();

							for(size_t i = 0; i < Parameters.size(); ++i)
							{
								if(funcsig.GetParameter(i).IsReference)
								{
									// TODO - bulletproof this a bit
									std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[i]->GetAtoms()[0]));
									Parameters[i]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier());
									Parameters[i]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, i);
								}
							}

							break;
						}
					}
				}
			}
			else
			{
				FunctionSignatureSet::const_iterator funciter = program.Session.FunctionSignatures.find(Name);
				if(funciter != program.Session.FunctionSignatures.end())
				{
					if(funciter->second.GetNumParameters() == Parameters.size())
					{
						bool match = true;
						for(size_t i = 0; i < Parameters.size(); ++i)
						{
							if(funciter->second.GetParameter(i).Type != Parameters[i]->GetEpochType(program))
							{
								match = false;
								break;
							}

							if(funciter->second.GetParameter(i).IsReference)
							{
								// TODO - bulletproof this a bit
								std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[i]->GetAtoms()[0]));
								Parameters[i]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier());
								Parameters[i]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, i);
							}
						}

						if(match)
							MyType = funciter->second.GetReturnType();
					}
				}
				else
				{
					Function* func = program.GetFunctions().find(context.FunctionName)->second;
					MyType = func->GetParameterSignatureType(Name, program);
				}
			}
		}
	}

	return MyType != VM::EpochType_Infer;
}


bool PreOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext&)
{
	VM::EpochTypeID operandtype = InferMemberAccessType(Operand, program, activescope);
	if(operandtype == VM::EpochType_Error)
		return false;

	StringHandleSet functionstocheck;

	const OverloadMap& overloads = *program.Session.InfoTable.Overloads;
	OverloadMap::const_iterator iter = overloads.find(OperatorName);
	if(iter == overloads.end())
		functionstocheck.insert(OperatorName);
	else
		functionstocheck = iter->second;

	if(functionstocheck.empty())
	{
		//
		// This is probably a failure of the library or whoever
		// wrote the preop implementation. The operator has been
		// registered in the grammar but not the overloads table,
		// so we can't figure out what parameters it takes.
		//
		throw InternalException("Preoperator defined in the grammar but no implementations could be located");
	}

	for(StringHandleSet::const_iterator iter = functionstocheck.begin(); iter != functionstocheck.end(); ++iter)
	{
		FunctionSignatureSet::const_iterator sigiter = program.Session.FunctionSignatures.find(*iter);
		if(sigiter == program.Session.FunctionSignatures.end())
		{
			//
			// This is another failure of the operator implementation.
			// The signature for the operator cannot be found.
			//
			throw InternalException("Preoperator defined but no signature provided");
		}

		if(sigiter->second.GetNumParameters() == 1 && sigiter->second.GetParameter(0).Type == operandtype)
		{
			OperatorName = *iter;
			MyType = sigiter->second.GetReturnType();
			break;
		}
	}

	return (MyType != VM::EpochType_Error);
}

bool PreOpStatement::Validate(const Program&) const
{
	return MyType != VM::EpochType_Error;
}


bool PostOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext&)
{
	VM::EpochTypeID operandtype = InferMemberAccessType(Operand, program, activescope);
	if(operandtype == VM::EpochType_Error)
		return false;

	StringHandleSet functionstocheck;

	const OverloadMap& overloads = *program.Session.InfoTable.Overloads;
	OverloadMap::const_iterator iter = overloads.find(OperatorName);
	if(iter == overloads.end())
		functionstocheck.insert(OperatorName);
	else
		functionstocheck = iter->second;

	if(functionstocheck.empty())
	{
		//
		// This is probably a failure of the library or whoever
		// wrote the postop implementation. The operator has been
		// registered in the grammar but not the overloads table,
		// so we can't figure out what parameters it takes.
		//
		throw InternalException("Postoperator defined in the grammar but no implementations could be located");
	}

	for(StringHandleSet::const_iterator iter = functionstocheck.begin(); iter != functionstocheck.end(); ++iter)
	{
		FunctionSignatureSet::const_iterator sigiter = program.Session.FunctionSignatures.find(*iter);
		if(sigiter == program.Session.FunctionSignatures.end())
		{
			//
			// This is another failure of the operator implementation.
			// The signature for the operator cannot be found.
			//
			throw InternalException("Postoperator defined but no signature provided");
		}

		if(sigiter->second.GetNumParameters() == 1 && sigiter->second.GetParameter(0).Type == operandtype)
		{
			OperatorName = *iter;
			MyType = sigiter->second.GetReturnType();
			break;
		}
	}

	return (MyType != VM::EpochType_Error);
}

bool PostOpStatement::Validate(const Program&) const
{
	return MyType != VM::EpochType_Error;
}
