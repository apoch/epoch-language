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
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Session.h"
#include "Compiler/Exceptions.h"

#include "User Interface/Output.h"


using namespace IRSemantics;


Statement::Statement(StringHandle name, const AST::IdentifierT& identifier)
	: Name(name),
	  RawName(name),
	  MyType(VM::EpochType_Error),
	  OriginalIdentifier(identifier),
	  CompileTimeCodeExecuted(false)
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

	valid = valid && (MyType != VM::EpochType_Infer) && (MyType != VM::EpochType_Error);
	return valid;
}

bool Statement::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	if(CompileTimeCodeExecuted)
		return true;

	CompileTimeCodeExecuted = true;

	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, activescope, inreturnexpr, errors))
			return false;
	}

	errors.SetContext(OriginalIdentifier);
	FunctionCompileHelperTable::const_iterator fchiter = program.Session.InfoTable.FunctionHelpers->find(Name);
	if(fchiter != program.Session.InfoTable.FunctionHelpers->end())
		fchiter->second(*this, program, activescope, inreturnexpr, errors);

	return true;
}

bool Statement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, CompileErrors& errors)
{
	if(MyType != VM::EpochType_Error)
		return (MyType != VM::EpochType_Infer);

	InferenceContext newcontext(Name, InferenceContext::CONTEXT_STATEMENT);
	newcontext.FunctionName = context.FunctionName;

	errors.SetContext(OriginalIdentifier);

	switch(context.State)
	{
	case InferenceContext::CONTEXT_CODE_BLOCK:
	case InferenceContext::CONTEXT_EXPRESSION:
	case InferenceContext::CONTEXT_FUNCTION_RETURN:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(Name, *activescope.GetScope(), context.FunctionName, errors));
		newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(Name, *activescope.GetScope(), context.FunctionName, errors));
		break;

	case InferenceContext::CONTEXT_STATEMENT:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(context.ContextName, *activescope.GetScope(), context.FunctionName, errors));
		newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(context.ContextName, *activescope.GetScope(), context.FunctionName, errors));
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

	if(context.State == InferenceContext::CONTEXT_FUNCTION_RETURN)
	{
		MyType = program.LookupType(Name);
		if(VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_Structure)
			Name = program.GetStructures().find(program.GetNameOfStructureType(MyType))->second->GetConstructorName();
		else if(VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_Unit)
			Name = program.StrongTypeAliasRepNames.find(MyType)->second;

		program.GetFunctions().find(context.FunctionName)->second->SetHintReturnType(MyType);
	}

	size_t i = 0;
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, newcontext, i, Parameters.size(), errors))
			return false;

		++i;
	}

	if(
		   context.State != InferenceContext::CONTEXT_FUNCTION_RETURN
		|| VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_Structure
	  )
	{
		if(program.HasFunction(Name))
		{
			unsigned totalexpectedpermutations = 1;
			bool generatetypematch = false;
			bool preferpatternmatchoverloads = false;
			std::map<StringHandle, FunctionSignature> matchingoverloads;

			InferenceContext funccontext(0, InferenceContext::CONTEXT_GLOBAL);
			unsigned overloadcount = program.GetNumFunctionOverloads(Name);
			for(unsigned i = 0; i < overloadcount; ++i)
			{
				unsigned expectedpermutations = 1;
	
				StringHandle overloadname = program.GetFunctionOverloadName(Name, i);
				VM::EpochTypeID funcreturntype = VM::EpochType_Error;
				FunctionSignature signature;
				if(program.Session.FunctionSignatures.find(overloadname) != program.Session.FunctionSignatures.end())
					signature = program.Session.FunctionSignatures.find(overloadname)->second;
				else
				{
					Function* func = program.GetFunctions().find(overloadname)->second;
					func->TypeInference(program, funccontext, errors);
					signature = func->GetFunctionSignature(program);
				}
				funcreturntype = signature.GetReturnType();
				if(signature.GetNumParameters() != Parameters.size())
					continue;

				VM::EpochTypeID underlyingreturntype = funcreturntype;
				if(VM::GetTypeFamily(funcreturntype) == VM::EpochTypeFamily_Unit)
				{
					if(program.StrongTypeAliasRepresentations.find(funcreturntype) != program.StrongTypeAliasRepresentations.end())
						underlyingreturntype = program.StrongTypeAliasRepresentations[funcreturntype];
				}


				if(!context.ExpectedTypes.empty())
				{
					bool matchedreturn = false;
					const InferenceContext::PossibleParameterTypes& possibles = context.ExpectedTypes.back();
					for(size_t j = 0; j < possibles.size(); ++j)
					{
						if(possibles[j].size() <= index)
							continue;

						if(possibles[j][index] == funcreturntype || possibles[j][index] == underlyingreturntype)
						{
							matchedreturn = true;
							break;
						}
					}

					if(!matchedreturn)
						continue;
				}
				
				bool match = true;
				for(size_t j = 0; j < Parameters.size(); ++j)
				{
					VM::EpochTypeID expectedparamtype = signature.GetParameter(j).Type;
					VM::EpochTypeID providedparamtype = Parameters[j]->GetEpochType(program);
					if(expectedparamtype != providedparamtype)
					{
						if(VM::GetTypeFamily(expectedparamtype) == VM::EpochTypeFamily_SumType)
						{
							if(!program.SumTypeHasTypeAsBase(expectedparamtype, providedparamtype))
							{
								match = false;
								break;
							}
							else if(VM::GetTypeFamily(providedparamtype) != VM::EpochTypeFamily_SumType)
								Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(providedparamtype));
						}
						else if(VM::GetTypeFamily(providedparamtype) == VM::EpochTypeFamily_SumType)
						{
							if(program.SumTypeHasTypeAsBase(providedparamtype, expectedparamtype))
							{
								generatetypematch = true;
								expectedpermutations *= program.GetNumSumTypeBases(providedparamtype);
							}
						}
						else
						{
							match = false;
							break;
						}
					}
					else if(expectedparamtype == VM::EpochType_Function)
					{
						if(Parameters[j]->GetAtoms().size() == 1)
						{
							ExpressionAtomIdentifier* atom = dynamic_cast<ExpressionAtomIdentifier*>(Parameters[j]->GetAtoms().front());
							if(!atom)
							{
								match = false;
								break;
							}

							StringHandle resolvedidentifier;
							unsigned signaturematches = program.FindMatchingFunctions(atom->GetIdentifier(), signature.GetFunctionSignature(j), context, errors, resolvedidentifier);
							if(signaturematches != 1)
							{
								match = false;
								break;
							}
						}
						else
						{
							// So, yeahhh... we're just gonna assume that type inference
							// of the parameter atoms already matched us a good function
							// here. It's probably something like a structure member, or
							// an indirect reference of that nature. We want to avoid an
							// ugly duplication of all the type inference logic here, so
							// we just guess that everything is fine and carry on.
						}
					}

					if(signature.GetParameter(j).HasPayload)
					{
						if(Parameters[j]->GetAtoms().size() == 1)
						{
							if(!signature.GetParameter(j).PatternMatch(Parameters[j]->GetAtoms()[0]->ConvertToCompileTimeParam(program)))
							{
								match = false;
								break;
							}
							Parameters[j]->AtomsArePatternMatchedLiteral = true;
							preferpatternmatchoverloads = true;
						}
						else
						{
							match = false;
							break;
						}
					}
				}

				if(match)
				{
					matchingoverloads.insert(std::make_pair(overloadname, signature));
					totalexpectedpermutations = expectedpermutations;
				}
			}

			if(generatetypematch && matchingoverloads.size() < totalexpectedpermutations)
			{
				errors.SetContext(OriginalIdentifier);
				errors.SemanticError("Missing at least one overload for handling sum type decomposition");
			}

			if(preferpatternmatchoverloads && matchingoverloads.size() > 1)
			{
				for(std::map<StringHandle, FunctionSignature>::iterator stripiter = matchingoverloads.begin(); stripiter != matchingoverloads.end(); )
				{
					bool stripoverload = false;
					for(size_t j = 0; j < stripiter->second.GetNumParameters(); ++j)
					{
						if(stripiter->second.GetParameter(j).HasPayload != Parameters[j]->AtomsArePatternMatchedLiteral)
						{
							stripoverload = true;
							break;
						}
					}

					if(stripoverload)
						stripiter = matchingoverloads.erase(stripiter);
					else
						++stripiter;
				}
			}

			if(matchingoverloads.size() == 1)
			{					
				Name = matchingoverloads.begin()->first;
				MyType = matchingoverloads.begin()->second.GetReturnType();

				for(size_t j = 0; j < Parameters.size(); ++j)
				{
					if(matchingoverloads.begin()->second.GetParameter(j).IsReference)
					{
						std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[j]->GetAtoms()[0]));
						if(atom.get())
						{
							Parameters[j]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
							Parameters[j]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, j, Parameters.size(), errors);
						}
						else
						{
							errors.SetContext(OriginalIdentifier);
							errors.SemanticError("Parameter expects a reference, not a literal");
						}
					}
				}
			}
			else if(generatetypematch)
			{
				std::vector<bool> paramsarereferences(Parameters.size(), false);

				MyType = matchingoverloads.begin()->second.GetReturnType();
				for(std::map<StringHandle, FunctionSignature>::const_iterator matchiter = matchingoverloads.begin(); matchiter != matchingoverloads.end(); ++matchiter)
				{
					if(matchiter->second.GetReturnType() != MyType)
					{
						errors.SetContext(OriginalIdentifier);
						errors.SemanticError("A matching overload differs by return type, which will confuse run-time type dispatch");
					}

					for(size_t j = 0; j < Parameters.size(); ++j)
					{
						if(matchiter->second.GetParameter(j).IsReference != matchingoverloads.begin()->second.GetParameter(j).IsReference)
						{
							bool acceptsnothing = false;
							for(std::map<StringHandle, FunctionSignature>::const_iterator matchiterinner = matchingoverloads.begin(); matchiterinner != matchingoverloads.end(); ++matchiterinner)
							{
								if(matchiterinner->second.GetParameter(j).Type == VM::EpochType_Nothing)
								{
									acceptsnothing = true;
									break;
								}
							}
							
							if(!acceptsnothing)
							{
								errors.SetContext(OriginalIdentifier);
								errors.SemanticError("A matching overload differs in accepting a parameter by reference or value");
							}
						}

						if(matchiter->second.GetParameter(j).IsReference)
							paramsarereferences[j] = true;
					}
				}

				for(size_t j = 0; j < Parameters.size(); ++j)
				{
					if(paramsarereferences[j])
					{
						std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[j]->GetAtoms()[0]));
						if(atom.get())
						{
							Parameters[j]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
							Parameters[j]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, j, Parameters.size(), errors);
							if(Parameters[j]->GetEpochType(program) != VM::EpochType_Nothing)
								Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(VM::EpochType_RefFlag));
						}
						else
						{
							if(Parameters[j]->GetAtoms().size() > 1)
							{
								if(Parameters[j]->GetEpochType(program) != VM::EpochType_Nothing)
								{
									Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotationFromRegister);
									Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(VM::EpochType_RefFlag));
								}
							}
							else
							{
								errors.SetContext(OriginalIdentifier);
								errors.SemanticError("Parameter expects a reference, not a literal");
							}
						}
					}
					else if(VM::GetTypeFamily(Parameters[j]->GetEpochType(program)) != VM::EpochTypeFamily_SumType)
						Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(Parameters[j]->GetEpochType(program)));
				}

				Name = program.AllocateTypeMatcher(RawName, matchingoverloads);
			}

			if(MyType == VM::EpochType_Error || MyType == VM::EpochType_Infer)
			{
				errors.SetContext(OriginalIdentifier);
				errors.SemanticError("No matching overload");
			}
		}
		else
		{
			bool overloadfound = false;
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
							VM::EpochTypeID providedtype = Parameters[i]->GetEpochType(program);
							VM::EpochTypeID expectedtype = funcsig.GetParameter(i).Type;
							if(providedtype != expectedtype)
							{
								if(VM::GetTypeFamily(expectedtype) == VM::EpochTypeFamily_SumType)
								{
									if(!program.SumTypeHasTypeAsBase(expectedtype, providedtype))
									{
										match = false;
										break;
									}
									else if(VM::GetTypeFamily(providedtype) != VM::EpochTypeFamily_SumType)
										Parameters[i]->AddAtom(new ExpressionAtomTypeAnnotation(providedtype));
								}
								else
								{
									match = false;
									break;
								}
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
									std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[i]->GetAtoms()[0]));
									if(atom.get())
									{
										Parameters[i]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
										Parameters[i]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, i, Parameters.size(), errors);
									}
									else
									{
										errors.SetContext(OriginalIdentifier);
										errors.SemanticError("Parameter expects a reference, not a literal");
									}
								}
							}

							overloadfound = true;
							break;
						}
					}
				}
			}
			
			if(!overloadfound)
			{
				FunctionSignatureSet::const_iterator funciter = program.Session.FunctionSignatures.find(Name);
				if(funciter != program.Session.FunctionSignatures.end())
				{
					if(funciter->second.GetNumParameters() == Parameters.size())
					{
						bool match = true;
						for(size_t i = 0; i < Parameters.size(); ++i)
						{
							VM::EpochTypeID expectedparamtype = funciter->second.GetParameter(i).Type;
							VM::EpochTypeID providedparamtype = Parameters[i]->GetEpochType(program);
							if(expectedparamtype != providedparamtype)
							{
								if(VM::GetTypeFamily(expectedparamtype) == VM::EpochTypeFamily_SumType)
								{
									if(!program.SumTypeHasTypeAsBase(expectedparamtype, providedparamtype))
									{
										match = false;
										break;
									}
									else if(VM::GetTypeFamily(providedparamtype) != VM::EpochTypeFamily_SumType)
										Parameters[i]->AddAtom(new ExpressionAtomTypeAnnotation(providedparamtype));
								}
								else
								{
									match = false;
									break;
								}
							}
						}

						if(match)
						{
							MyType = funciter->second.GetReturnType();

							for(size_t i = 0; i < Parameters.size(); ++i)
							{
								if(funciter->second.GetParameter(i).IsReference)
								{
									std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[i]->GetAtoms()[0]));
									if(atom.get())
									{
										Parameters[i]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
										Parameters[i]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, i, Parameters.size(), errors);
									}
									else
									{
										errors.SetContext(OriginalIdentifier);
										errors.SemanticError("Parameter expects a reference, not a literal");
									}
								}
							}
						}
						else
						{
							errors.SetContext(OriginalIdentifier);
							errors.SemanticError("No matching overload");
						}
					}
				}
				else
				{
					Function* func = program.GetFunctions().find(context.FunctionName)->second;
					if(func->HasParameter(Name))
						MyType = func->GetParameterSignatureType(Name, program);
					else
					{
						errors.SetContext(OriginalIdentifier);
						errors.SemanticError("Undefined function");
					}
				}
			}
		}
	}

	Name = program.MapConstructorNameForSumType(Name);

	bool valid = (MyType != VM::EpochType_Infer && MyType != VM::EpochType_Error);
	if(!valid)
		return false;

	if(!CompileTimeCodeExecution(program, activescope, context.ContextName == InferenceContext::CONTEXT_FUNCTION_RETURN, errors))
		return false;

	return true;
}


bool PreOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext&, CompileErrors&)
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

		VM::EpochTypeID underlyingtype = operandtype;
		if(VM::GetTypeFamily(operandtype) == VM::EpochTypeFamily_Unit)
		{
			if(program.StrongTypeAliasRepresentations.find(operandtype) != program.StrongTypeAliasRepresentations.end())
				underlyingtype = program.StrongTypeAliasRepresentations[operandtype];
		}

		if(sigiter->second.GetNumParameters() == 1 && sigiter->second.GetParameter(0).Type == underlyingtype)
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


bool PostOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext&, CompileErrors&)
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

		VM::EpochTypeID underlyingtype = operandtype;
		if(VM::GetTypeFamily(operandtype) == VM::EpochTypeFamily_Unit)
		{
			if(program.StrongTypeAliasRepresentations.find(operandtype) != program.StrongTypeAliasRepresentations.end())
				underlyingtype = program.StrongTypeAliasRepresentations[operandtype];
		}

		if(sigiter->second.GetNumParameters() == 1 && sigiter->second.GetParameter(0).Type == underlyingtype)
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
