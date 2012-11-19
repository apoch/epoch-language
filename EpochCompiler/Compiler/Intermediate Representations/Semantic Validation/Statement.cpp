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


//
// Construct and initialize a statement IR node
//
Statement::Statement(StringHandle name, const AST::IdentifierT& identifier)
	: Name(name),
	  RawName(name),
	  MyType(Metadata::EpochType_Error),
	  OriginalIdentifier(identifier),
	  CompileTimeCodeExecuted(false),
	  NeedsInstantiation(false)
{
}

//
// Destruct and clean up a statement IR node
//
Statement::~Statement()
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete *iter;
}

//
// Add a parameter to a statement
//
void Statement::AddParameter(Expression* expression)
{
	Parameters.push_back(expression);
}

//
// Validate the contents of a statement for type correctness etc.
//
bool Statement::Validate(const Namespace& curnamespace) const
{
	bool valid = true;

	for(std::vector<Expression*>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->Validate(curnamespace))
			valid = false;
	}

	valid = valid && (MyType != Metadata::EpochType_Infer) && (MyType != Metadata::EpochType_Error);
	return valid;
}

//
// Perform compile-time code execution for a statement
//
// Maps constructor names for weak type aliases, ensures that
// templated function invocations are instantiated correctly,
// and invokes any appropriate compiler helper routines.
//
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

//
// Perform type inference on a statement
//
// Note that this is what actually initiates compile time code
// execution (i.e. when it's all done and successful) in order
// to ensure that variable construction etc. is done correctly
// using appropriate type information.
//
// This routine is fairly long and ugly because it has to take
// into consideration quite a few language features: templated
// functions and types, overloading, intrinsic functions, type
// dispatchers, pattern matchers, and so on.
//
bool Statement::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, size_t index, CompileErrors& errors)
{
	if(MyType != Metadata::EpochType_Error)
		return (MyType != Metadata::EpochType_Infer);

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
		if(Metadata::GetTypeFamily(MyType) == Metadata::EpochTypeFamily_Structure)
			Name = curnamespace.Types.Structures.GetConstructorName(Name);
		else if(Metadata::GetTypeFamily(MyType) == Metadata::EpochTypeFamily_TemplateInstance)
			Name = curnamespace.Types.Templates.FindConstructorName(Name);
		else if(Metadata::GetTypeFamily(MyType) == Metadata::EpochTypeFamily_Unit)
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
		|| Metadata::GetTypeFamily(MyType) == Metadata::EpochTypeFamily_Structure
		|| Metadata::GetTypeFamily(MyType) == Metadata::EpochTypeFamily_TemplateInstance
	  )
	{
		if(curnamespace.Functions.IRExists(RawName))
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
				Metadata::EpochTypeID funcreturntype = Metadata::EpochType_Error;
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

				Metadata::EpochTypeID underlyingreturntype = funcreturntype;
				if(Metadata::GetTypeFamily(funcreturntype) == Metadata::EpochTypeFamily_Unit)
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
					Metadata::EpochTypeID expectedparamtype = signature.GetParameter(j).Type;
					Metadata::EpochTypeID providedparamtype = Parameters[j]->GetEpochType(curnamespace);
					if(expectedparamtype != providedparamtype)
					{
						if(Metadata::GetTypeFamily(expectedparamtype) == Metadata::EpochTypeFamily_SumType)
						{
							if(!curnamespace.Types.SumTypes.IsBaseType(expectedparamtype, providedparamtype))
							{
								match = false;
								break;
							}
							else if(Metadata::GetTypeFamily(providedparamtype) != Metadata::EpochTypeFamily_SumType)
							{
								generatetypematch = true;
								expectedpermutations *= curnamespace.Types.SumTypes.GetNumBaseTypes(providedparamtype);
							}
						}
						else if(Metadata::GetTypeFamily(providedparamtype) == Metadata::EpochTypeFamily_SumType)
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
					else if(expectedparamtype == Metadata::EpochType_Function)
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
								if(matchiterinner->second.GetParameter(j).Type == Metadata::EpochType_Nothing)
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
							if(Parameters[j]->GetEpochType(curnamespace) != Metadata::EpochType_Nothing)
								Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(Metadata::EpochType_RefFlag));
						}
						else
						{
							if(Parameters[j]->GetAtoms().size() > 1)
							{
								if(Parameters[j]->GetEpochType(curnamespace) != Metadata::EpochType_Nothing)
								{
									Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotationFromRegister);
									Parameters[j]->AddAtom(new ExpressionAtomTypeAnnotation(Metadata::EpochType_RefFlag));
								}
							}
							else
							{
								errors.SetContext(OriginalIdentifier);
								errors.SemanticError("Parameter expects a reference, not a literal");
							}
						}
					}
					else if(Metadata::GetTypeFamily(Parameters[j]->GetEpochType(curnamespace)) != Metadata::EpochTypeFamily_SumType)
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

			if(MyType == Metadata::EpochType_Error || MyType == Metadata::EpochType_Infer)
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
							Metadata::EpochTypeID providedtype = Parameters[i]->GetEpochType(curnamespace);
							Metadata::EpochTypeID expectedtype = funcsig.GetParameter(i).Type;
							if(providedtype != expectedtype)
							{
								if(Metadata::GetTypeFamily(expectedtype) == Metadata::EpochTypeFamily_SumType)
								{
									if(!curnamespace.Types.SumTypes.IsBaseType(expectedtype, providedtype))
									{
										match = false;
										break;
									}
									else if(Metadata::GetTypeFamily(providedtype) != Metadata::EpochTypeFamily_SumType)
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
							Metadata::EpochTypeID expectedparamtype = signature.GetParameter(i).Type;
							Metadata::EpochTypeID providedparamtype = Parameters[i]->GetEpochType(curnamespace);
							if(expectedparamtype != providedparamtype)
							{
								if(Metadata::GetTypeFamily(expectedparamtype) == Metadata::EpochTypeFamily_SumType)
								{
									if(!curnamespace.Types.SumTypes.IsBaseType(expectedparamtype, providedparamtype))
									{
										match = false;
										break;
									}
									else if(Metadata::GetTypeFamily(providedparamtype) != Metadata::EpochTypeFamily_SumType)
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
						errors.SemanticError("No matching overload");
					}
				}
			}
		}
	}

	{
		StringHandle constructorname = curnamespace.Types.SumTypes.MapConstructorName(Name);
		if(constructorname != Name)
			Name = RawName = constructorname;
	}

	bool valid = (MyType != Metadata::EpochType_Infer && MyType != Metadata::EpochType_Error);
	if(!valid)
		return false;

	if(!CompileTimeCodeExecution(curnamespace, activescope, context.ContextName == InferenceContext::CONTEXT_FUNCTION_RETURN, errors))
		return false;

	if(!context.ExpectedTypes.empty() && MyType == Metadata::EpochType_Void)
	{
		errors.SetContext(OriginalIdentifier);
		errors.SemanticError("This function has no return value");
	}

	return true;
}

//
// Set template arguments for a statement
//
// Maps constructors to the correct names when template
// arguments are passed to a templated type constructor
//
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
			errors.SetContext(OriginalIdentifier);
			errors.SemanticError("This is not a template");
		}
	}
}

//
// Store template arguments for later instantiation
//
void Statement::SetTemplateArgsDeferred(const CompileTimeParameterVector& args)
{
	if(!args.empty())
	{
		TemplateArgs = args;
		NeedsInstantiation = true;
	}
}


//
// Perform type inference on a pre-operation statement
//
bool PreOpStatement::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext&, CompileErrors& errors)
{
	errors.SetContext(OriginalOperand);
	Metadata::EpochTypeID operandtype = InferMemberAccessType(Operand, curnamespace, activescope, errors);
	if(operandtype == Metadata::EpochType_Error)
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

		Metadata::EpochTypeID underlyingtype = operandtype;
		if(Metadata::GetTypeFamily(operandtype) == Metadata::EpochTypeFamily_Unit)
			underlyingtype = curnamespace.Types.Aliases.GetStrongRepresentation(operandtype);

		if(signature.GetNumParameters() == 1 && signature.GetParameter(0).Type == underlyingtype)
		{
			OperatorName = *iter;
			MyType = signature.GetReturnType();
			break;
		}
	}

	return (MyType != Metadata::EpochType_Error);
}

//
// Validate a pre-operation statement
//
bool PreOpStatement::Validate(const Namespace&) const
{
	return MyType != Metadata::EpochType_Error;
}


//
// Perform type inference on a post-operation statement
//
bool PostOpStatement::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext&, CompileErrors& errors)
{
	errors.SetContext(OriginalOperand);
	Metadata::EpochTypeID operandtype = InferMemberAccessType(Operand, curnamespace, activescope, errors);
	if(operandtype == Metadata::EpochType_Error)
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

		Metadata::EpochTypeID underlyingtype = operandtype;
		if(Metadata::GetTypeFamily(operandtype) == Metadata::EpochTypeFamily_Unit)
			underlyingtype = curnamespace.Types.Aliases.GetStrongRepresentation(operandtype);

		if(signature.GetNumParameters() == 1 && signature.GetParameter(0).Type == underlyingtype)
		{
			OperatorName = *iter;
			MyType = signature.GetReturnType();
			break;
		}
	}

	return (MyType != Metadata::EpochType_Error);
}

//
// Validate a post-operation statement
//
bool PostOpStatement::Validate(const Namespace&) const
{
	return MyType != Metadata::EpochType_Error;
}


//
// Deep-copy a statement IR node
//
Statement* Statement::Clone() const
{
	Statement* clone = new Statement(Name, OriginalIdentifier);
	clone->RawName = RawName;
	for(std::vector<Expression*>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		clone->Parameters.push_back((*iter)->Clone());
	clone->MyType = Metadata::EpochType_Error;
	clone->CompileTimeCodeExecuted = false;
	clone->TemplateArgs = TemplateArgs;
	clone->NeedsInstantiation = NeedsInstantiation;
	return clone;
}

//
// Deep-copy a pre-operation statement IR node
//
PreOpStatement* PreOpStatement::Clone() const
{
	PreOpStatement* clone = new PreOpStatement(OperatorName, Operand, OriginalOperand);
	clone->MyType = MyType;
	return clone;
}

//
// Deep-copy a post-operation statement IR node
//
PostOpStatement* PostOpStatement::Clone() const
{
	PostOpStatement* clone = new PostOpStatement(Operand, OperatorName, OriginalOperand);
	clone->MyType = MyType;
	return clone;
}

