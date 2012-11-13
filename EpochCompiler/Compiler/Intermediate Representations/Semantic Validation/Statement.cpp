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
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/CompileErrors.h"
#include "Compiler/Exceptions.h"


using namespace IRSemantics;


Statement::Statement(StringHandle name, const AST::IdentifierT& identifier)
	: Name(name),
	  RawName(name),
	  MyType(VM::EpochType_Error),
	  OriginalIdentifier(identifier),
	  CompileTimeCodeExecuted(false),
	  NeedsInstantiation(false)
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


bool Statement::Validate(const Namespace& curnamespace) const
{
	bool valid = true;

	for(std::vector<Expression*>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->Validate(curnamespace))
			valid = false;
	}

	valid = valid && (MyType != VM::EpochType_Infer) && (MyType != VM::EpochType_Error);
	return valid;
}

bool Statement::CompileTimeCodeExecution(Namespace& curnamespace, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	if(CompileTimeCodeExecuted)
		return true;

	CompileTimeCodeExecuted = true;

	if(NeedsInstantiation)
	{
		SetTemplateArgs(TemplateArgs, curnamespace, errors);
		NeedsInstantiation = false;
	}


	if(curnamespace.Types.Aliases.HasWeakAliasNamed(Name))
		Name = curnamespace.Types.Aliases.GetWeakTypeBaseName(Name);

	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(curnamespace, activescope, inreturnexpr, errors))
			return false;
	}

	errors.SetContext(OriginalIdentifier);
	if(curnamespace.Functions.HasCompileHelper(Name))
		curnamespace.Functions.GetCompileHelper(Name)(*this, curnamespace, activescope, inreturnexpr, errors);

	return true;
}

bool Statement::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, CompileErrors& errors)
{
	if(MyType != VM::EpochType_Error)
		return (MyType != VM::EpochType_Infer);

	if(NeedsInstantiation)
	{
		SetTemplateArgs(TemplateArgs, curnamespace, errors);
		NeedsInstantiation = false;
	}

	InferenceContext newcontext(Name, InferenceContext::CONTEXT_STATEMENT);
	newcontext.FunctionName = context.FunctionName;

	errors.SetContext(OriginalIdentifier);

	switch(context.State)
	{
	case InferenceContext::CONTEXT_CODE_BLOCK:
	case InferenceContext::CONTEXT_EXPRESSION:
	case InferenceContext::CONTEXT_FUNCTION_RETURN:
		newcontext.ExpectedTypes.push_back(curnamespace.Functions.GetExpectedTypes(Name, *activescope.GetScope(), context.FunctionName, errors));
		newcontext.ExpectedSignatures.push_back(curnamespace.Functions.GetExpectedSignatures(Name, *activescope.GetScope(), context.FunctionName, errors));
		break;

	case InferenceContext::CONTEXT_STATEMENT:
		newcontext.ExpectedTypes.push_back(curnamespace.Functions.GetExpectedTypes(context.ContextName, *activescope.GetScope(), context.FunctionName, errors));
		newcontext.ExpectedSignatures.push_back(curnamespace.Functions.GetExpectedSignatures(context.ContextName, *activescope.GetScope(), context.FunctionName, errors));
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
		MyType = curnamespace.Types.GetTypeByName(Name);
		if(VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_Structure)
			Name = curnamespace.Types.Structures.GetConstructorName(Name);
		else if(VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_TemplateInstance)
			Name = curnamespace.Types.Templates.FindConstructorName(Name);
		else if(VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_Unit)
			Name = curnamespace.Types.Aliases.GetStrongRepresentationName(MyType);

		curnamespace.Functions.GetIR(context.FunctionName)->SetHintReturnType(MyType);
	}

	size_t i = 0;
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->TypeInference(curnamespace, activescope, newcontext, i, Parameters.size(), errors))
			return false;

		++i;
	}

	if(
		   context.State != InferenceContext::CONTEXT_FUNCTION_RETURN
		|| VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_Structure
		|| VM::GetTypeFamily(MyType) == VM::EpochTypeFamily_TemplateInstance
	  )
	{
		if(curnamespace.Functions.Exists(RawName))
		{
			unsigned totalexpectedpermutations = 1;
			bool generatetypematch = false;
			bool preferpatternmatchoverloads = false;
			std::map<StringHandle, FunctionSignature> matchingoverloads;
			std::map<StringHandle, FunctionSignature> overloadswithsameparamcount;

			InferenceContext funccontext(0, InferenceContext::CONTEXT_GLOBAL);
			unsigned overloadcount = curnamespace.Functions.GetNumOverloads(RawName);
			for(unsigned i = 0; i < overloadcount; ++i)
			{
				unsigned expectedpermutations = 1;
	
				StringHandle overloadname = curnamespace.Functions.GetOverloadName(RawName, i);
				VM::EpochTypeID funcreturntype = VM::EpochType_Error;
				FunctionSignature signature;
				if(curnamespace.Functions.SignatureExists(overloadname))
					signature = curnamespace.Functions.GetSignature(overloadname);
				else
				{
					Function* func = curnamespace.Functions.GetIR(overloadname);
					if(func->IsTemplate())
						continue;

					func->TypeInference(curnamespace, funccontext, errors);
					signature = func->GetFunctionSignature(curnamespace);
				}
				funcreturntype = signature.GetReturnType();
				if(signature.GetNumParameters() != Parameters.size())
					continue;

				VM::EpochTypeID underlyingreturntype = funcreturntype;
				if(VM::GetTypeFamily(funcreturntype) == VM::EpochTypeFamily_Unit)
					underlyingreturntype = curnamespace.Types.Aliases.GetStrongRepresentation(funcreturntype);

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

				overloadswithsameparamcount.insert(std::make_pair(overloadname, signature));
				
				bool match = true;
				for(size_t j = 0; j < Parameters.size(); ++j)
				{
					VM::EpochTypeID expectedparamtype = signature.GetParameter(j).Type;
					VM::EpochTypeID providedparamtype = Parameters[j]->GetEpochType(curnamespace);
					if(expectedparamtype != providedparamtype)
					{
						if(VM::GetTypeFamily(expectedparamtype) == VM::EpochTypeFamily_SumType)
						{
							if(!curnamespace.Types.SumTypes.IsBaseType(expectedparamtype, providedparamtype))
							{
								match = false;
								break;
							}
							else if(VM::GetTypeFamily(providedparamtype) != VM::EpochTypeFamily_SumType)
							{
								generatetypematch = true;
								expectedpermutations *= curnamespace.Types.SumTypes.GetNumBaseTypes(providedparamtype);
							}
						}
						else if(VM::GetTypeFamily(providedparamtype) == VM::EpochTypeFamily_SumType)
						{
							if(curnamespace.Types.SumTypes.IsBaseType(providedparamtype, expectedparamtype))
							{
								generatetypematch = true;
								expectedpermutations *= curnamespace.Types.SumTypes.GetNumBaseTypes(providedparamtype);
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
							unsigned signaturematches = curnamespace.Functions.FindMatchingFunctions(atom->GetIdentifier(), signature.GetFunctionSignature(j), context, errors, resolvedidentifier);
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
							if(!signature.GetParameter(j).PatternMatch(Parameters[j]->GetAtoms()[0]->ConvertToCompileTimeParam(curnamespace)))
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

			// TODO - this check is WEAK SAUCE. Improve it somehow.
			if(generatetypematch && overloadcount < totalexpectedpermutations)
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

			if(generatetypematch)
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
							Parameters[j]->GetAtoms()[0]->TypeInference(curnamespace, activescope, newcontext, j, Parameters.size(), errors);
							if(Parameters[j]->GetEpochType(curnamespace) != VM::EpochType_Nothing)
								Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(VM::EpochType_RefFlag));
						}
						else
						{
							if(Parameters[j]->GetAtoms().size() > 1)
							{
								if(Parameters[j]->GetEpochType(curnamespace) != VM::EpochType_Nothing)
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
					else if(VM::GetTypeFamily(Parameters[j]->GetEpochType(curnamespace)) != VM::EpochTypeFamily_SumType)
						Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(Parameters[j]->GetEpochType(curnamespace)));
				}

				Name = curnamespace.Functions.AllocateTypeMatcher(RawName, overloadswithsameparamcount);
			}
			else if(matchingoverloads.size() == 1)
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
							Parameters[j]->GetAtoms()[0]->TypeInference(curnamespace, activescope, newcontext, j, Parameters.size(), errors);
						}
						else
						{
							errors.SetContext(OriginalIdentifier);
							errors.SemanticError("Parameter expects a reference, not a literal");
						}
					}
				}
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
			if(curnamespace.Functions.HasOverloads(Name))
			{
				const StringHandleSet& overloads = curnamespace.Functions.GetOverloadNames(Name);
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& funcsig = curnamespace.Functions.GetSignature(*overloaditer);
					if(funcsig.GetNumParameters() == Parameters.size())
					{
						bool match = true;
						for(size_t i = 0; i < Parameters.size(); ++i)
						{
							VM::EpochTypeID providedtype = Parameters[i]->GetEpochType(curnamespace);
							VM::EpochTypeID expectedtype = funcsig.GetParameter(i).Type;
							if(providedtype != expectedtype)
							{
								if(VM::GetTypeFamily(expectedtype) == VM::EpochTypeFamily_SumType)
								{
									if(!curnamespace.Types.SumTypes.IsBaseType(expectedtype, providedtype))
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
										Parameters[i]->GetAtoms()[0]->TypeInference(curnamespace, activescope, newcontext, i, Parameters.size(), errors);
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
				if(curnamespace.Functions.SignatureExists(Name))
				{
					const FunctionSignature& signature = curnamespace.Functions.GetSignature(Name);
					if(signature.GetNumParameters() == Parameters.size())
					{
						bool match = true;
						for(size_t i = 0; i < Parameters.size(); ++i)
						{
							VM::EpochTypeID expectedparamtype = signature.GetParameter(i).Type;
							VM::EpochTypeID providedparamtype = Parameters[i]->GetEpochType(curnamespace);
							if(expectedparamtype != providedparamtype)
							{
								if(VM::GetTypeFamily(expectedparamtype) == VM::EpochTypeFamily_SumType)
								{
									if(!curnamespace.Types.SumTypes.IsBaseType(expectedparamtype, providedparamtype))
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
							MyType = signature.GetReturnType();

							for(size_t i = 0; i < Parameters.size(); ++i)
							{
								if(signature.GetParameter(i).IsReference)
								{
									std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[i]->GetAtoms()[0]));
									if(atom.get())
									{
										Parameters[i]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
										Parameters[i]->GetAtoms()[0]->TypeInference(curnamespace, activescope, newcontext, i, Parameters.size(), errors);
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
					Function* func = curnamespace.Functions.GetIR(context.FunctionName);
					if(func->HasParameter(Name))
						MyType = func->GetParameterSignatureType(Name, curnamespace);
					else
					{
						errors.SetContext(OriginalIdentifier);
						errors.SemanticError("Undefined function");
					}
				}
			}
		}
	}

	// TODO - this doesn't respect template arguments!
	Name = curnamespace.Types.SumTypes.MapConstructorName(Name);

	bool valid = (MyType != VM::EpochType_Infer && MyType != VM::EpochType_Error);
	if(!valid)
		return false;

	if(!CompileTimeCodeExecution(curnamespace, activescope, context.ContextName == InferenceContext::CONTEXT_FUNCTION_RETURN, errors))
		return false;

	return true;
}


void Statement::SetTemplateArgs(const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors)
{
	if(!args.empty())
	{
		if(curnamespace.Types.Structures.IsStructureTemplate(Name))
		{
			RawName = curnamespace.Types.Templates.InstantiateStructure(Name, args);
			Name = curnamespace.Types.Templates.FindConstructorName(RawName);
		}
		else if(curnamespace.Types.SumTypes.IsTemplate(Name))
		{
			RawName = curnamespace.Types.SumTypes.InstantiateTemplate(Name, args);
			Name = RawName;
		}
		else if(curnamespace.Functions.IsFunctionTemplate(Name))
		{
			Name = curnamespace.Functions.InstantiateAllOverloads(Name, args, errors);
			TemplateArgs = args;
		}
		else
		{
			// TODO - document exception
			throw InternalException("Template arguments provided in unrecognized context");
		}
	}
}

void Statement::SetTemplateArgsDeferred(const CompileTimeParameterVector& args)
{
	if(!args.empty())
	{
		TemplateArgs = args;
		NeedsInstantiation = true;
	}
}


bool PreOpStatement::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext&, CompileErrors&)
{
	VM::EpochTypeID operandtype = InferMemberAccessType(Operand, curnamespace, activescope);
	if(operandtype == VM::EpochType_Error)
		return false;

	StringHandleSet functionstocheck;

	if(curnamespace.Functions.HasOverloads(OperatorName))
		functionstocheck = curnamespace.Functions.GetOverloadNames(OperatorName);
	else
		functionstocheck.insert(OperatorName);

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
		if(!curnamespace.Functions.SignatureExists(*iter))
		{
			//
			// This is another failure of the operator implementation.
			// The signature for the operator cannot be found.
			//
			throw InternalException("Preoperator defined but no signature provided");
		}

		const FunctionSignature& signature = curnamespace.Functions.GetSignature(*iter);

		VM::EpochTypeID underlyingtype = operandtype;
		if(VM::GetTypeFamily(operandtype) == VM::EpochTypeFamily_Unit)
			underlyingtype = curnamespace.Types.Aliases.GetStrongRepresentation(operandtype);

		if(signature.GetNumParameters() == 1 && signature.GetParameter(0).Type == underlyingtype)
		{
			OperatorName = *iter;
			MyType = signature.GetReturnType();
			break;
		}
	}

	return (MyType != VM::EpochType_Error);
}

bool PreOpStatement::Validate(const Namespace&) const
{
	return MyType != VM::EpochType_Error;
}


bool PostOpStatement::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext&, CompileErrors&)
{
	VM::EpochTypeID operandtype = InferMemberAccessType(Operand, curnamespace, activescope);
	if(operandtype == VM::EpochType_Error)
		return false;

	StringHandleSet functionstocheck;

	if(curnamespace.Functions.HasOverloads(OperatorName))
		functionstocheck = curnamespace.Functions.GetOverloadNames(OperatorName);
	else
		functionstocheck.insert(OperatorName);

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
		if(!curnamespace.Functions.SignatureExists(*iter))
		{
			//
			// This is another failure of the operator implementation.
			// The signature for the operator cannot be found.
			//
			throw InternalException("Postoperator defined but no signature provided");
		}

		const FunctionSignature& signature = curnamespace.Functions.GetSignature(*iter);

		VM::EpochTypeID underlyingtype = operandtype;
		if(VM::GetTypeFamily(operandtype) == VM::EpochTypeFamily_Unit)
			underlyingtype = curnamespace.Types.Aliases.GetStrongRepresentation(operandtype);

		if(signature.GetNumParameters() == 1 && signature.GetParameter(0).Type == underlyingtype)
		{
			OperatorName = *iter;
			MyType = signature.GetReturnType();
			break;
		}
	}

	return (MyType != VM::EpochType_Error);
}

bool PostOpStatement::Validate(const Namespace&) const
{
	return MyType != VM::EpochType_Error;
}



Statement* Statement::Clone() const
{
	Statement* clone = new Statement(Name, OriginalIdentifier);
	clone->RawName = RawName;
	for(std::vector<Expression*>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		clone->Parameters.push_back((*iter)->Clone());
	clone->MyType = VM::EpochType_Error;
	clone->CompileTimeCodeExecuted = false;
	clone->TemplateArgs = TemplateArgs;
	clone->NeedsInstantiation = NeedsInstantiation;
	return clone;
}

PreOpStatement* PreOpStatement::Clone() const
{
	PreOpStatement* clone = new PreOpStatement(OperatorName, Operand);
	clone->MyType = MyType;
	return clone;
}

PostOpStatement* PostOpStatement::Clone() const
{
	PostOpStatement* clone = new PostOpStatement(Operand, OperatorName);
	clone->MyType = MyType;
	return clone;
}

