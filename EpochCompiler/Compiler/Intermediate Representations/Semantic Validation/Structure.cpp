//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a structure definition
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"

#include "Compiler/CompileErrors.h"

#include "Utility/StringPool.h"


using namespace IRSemantics;


//
// Destruct and clean up a structure definition wrapper
//
Structure::~Structure()
{
	for(std::vector<std::pair<StringHandle, StructureMember*> >::iterator iter = Members.begin(); iter != Members.end(); ++iter)
		delete iter->second;
}

//
// Add a member to a structure definition
//
void Structure::AddMember(StringHandle name, StructureMember* member, CompileErrors& errors)
{
	for(std::vector<std::pair<StringHandle, StructureMember*> >::iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(name == iter->first)
		{
			delete member;
			errors.SemanticError("Duplicate structure member name");
			return;
		}
	}

	Members.push_back(std::make_pair(name, member));
}

//
// Validate a structure definition
//
bool Structure::Validate(const Namespace& curnamespace, CompileErrors& errors) const
{
	if(IsTemplate())
		return true;

	bool valid = true;

	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(!iter->second->Validate(curnamespace, errors))
			valid = false;
	}

	return valid;
}

//
// Generate constructors for a given structure
//
// Separated into its own routine to aid in template generation
//
void Structure::GenerateConstructors(StringHandle myname, StringHandle constructorname, StringHandle anonconstructorname, StringHandle copyconstructorname, const CompileTimeParameterVector& templateargs, Namespace& curnamespace, CompileErrors& errors) const
{
	// Generate standard constructor
	{
		size_t i = 0;
		FunctionSignature signature;
		signature.AddParameter(L"id", Metadata::MakeReferenceType(Metadata::EpochType_Identifier));
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
		{
			iter->second->PopulateTypeSpace(curnamespace);
			Metadata::EpochTypeID paramtype = IsTemplate() ? SubstituteTemplateParams(iter->first, templateargs, curnamespace) : iter->second->GetEpochType(curnamespace);

			signature.AddParameter(curnamespace.Strings.GetPooledString(iter->first), paramtype);
			++i;

			if(iter->second->GetMemberType() == StructureMember::FunctionReference)
			{
				const StructureMemberFunctionReference* funcref = dynamic_cast<const StructureMemberFunctionReference*>(iter->second);
				signature.SetFunctionSignature(i, funcref->GetSignature(curnamespace));
			}
		}

		curnamespace.Functions.SetSignature(constructorname, signature);
		curnamespace.Functions.SetCompileHelper(constructorname, &CompileConstructorHelper);
	}

	// Generate anonymous constructor
	{
		size_t i = 0;
		FunctionSignature signature;
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
		{
			Metadata::EpochTypeID paramtype = IsTemplate() ? SubstituteTemplateParams(iter->first, templateargs, curnamespace) : iter->second->GetEpochType(curnamespace);

			signature.AddParameter(curnamespace.Strings.GetPooledString(iter->first), paramtype);

			if(iter->second->GetMemberType() == StructureMember::FunctionReference)
			{
				const StructureMemberFunctionReference* funcref = dynamic_cast<const StructureMemberFunctionReference*>(iter->second);
				signature.SetFunctionSignature(i, funcref->GetSignature(curnamespace));
			}

			++i;
		}
		signature.SetReturnType(curnamespace.Types.GetTypeByName(myname));

		curnamespace.Functions.SetSignature(anonconstructorname, signature);
	}

	curnamespace.Functions.AddInternalOverload(myname, anonconstructorname);


	// Generate copy constructor
	if(copyconstructorname)
	{
		FunctionSignature signature;
		signature.AddParameter(L"id", Metadata::MakeReferenceType(Metadata::EpochType_Identifier));
		signature.AddParameter(L"value", curnamespace.Types.GetTypeByName(myname));

		curnamespace.Strings.Pool(L"value");

		curnamespace.Functions.SetSignature(copyconstructorname, signature);
		curnamespace.Functions.SetCompileHelper(copyconstructorname, &CompileConstructorHelper);
	}

	curnamespace.Functions.AddInternalOverload(myname, copyconstructorname);


	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		StringHandle funcname = curnamespace.Functions.FindStructureMemberAccessOverload(myname, iter->first);

		Metadata::EpochTypeID type = IsTemplate() ? SubstituteTemplateParams(iter->first, templateargs, curnamespace) : iter->second->GetEpochType(curnamespace);

		std::auto_ptr<Function> func(new Function);
		std::auto_ptr<Expression> retexpr(new Expression);
		retexpr->AddAtom(new ExpressionAtomCopyFromStructure(type, iter->first));
		func->SetReturnExpression(retexpr.release());

		std::auto_ptr<ScopeDescription> scope(new ScopeDescription(curnamespace.GetGlobalScope()));
		scope->AddVariable(L"identifier", curnamespace.Strings.Pool(L"identifier"), myname, Metadata::MakeReferenceType(curnamespace.Types.GetTypeByName(myname)), VARIABLE_ORIGIN_PARAMETER);
		scope->AddVariable(L"ret", curnamespace.Strings.Pool(L"ret"), curnamespace.Types.GetNameOfType(type), type, VARIABLE_ORIGIN_RETURN);
		std::auto_ptr<CodeBlock> codeblock(new CodeBlock(scope.release()));
		curnamespace.AllocateLexicalScopeName(codeblock->GetScope());
		func->SetCode(codeblock.release());
		func->SetName(funcname);
		func->AddParameter(curnamespace.Strings.Find(L"identifier"), new FunctionParamTyped(Metadata::MakeReferenceType(type)), (type == Metadata::EpochType_Nothing), errors);
		func->SuppressReturnRegister();
		func->SuppressCodeEmission();

		curnamespace.Functions.Add(funcname, funcname, func.release());
	}
}


//
// Retrieve the type of a member, once it has been instantiated
// with the given template arguments
//
Metadata::EpochTypeID Structure::SubstituteTemplateParams(StringHandle membername, const CompileTimeParameterVector& templateargs, Namespace& curnamespace) const
{
	if(!IsTemplate())
		return Metadata::EpochType_Error;

	if(templateargs.size() != TemplateParams.size())
		return Metadata::EpochType_Error;

	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(iter->first == membername)
		{
			if(iter->second->GetMemberType() == StructureMember::Variable)
			{
				const StructureMemberVariable* member = dynamic_cast<const StructureMemberVariable*>(iter->second);
				if(member)
				{
					for(size_t i = 0; i < TemplateParams.size(); ++i)
					{
						if(member->GetOriginalNameOfType() == TemplateParams[i].first)
							return curnamespace.Types.GetTypeByName(templateargs[i].Payload.LiteralStringHandleValue);
					}

					CompileErrors errors;		// TODO - hack
					return curnamespace.Types.GetTypeByName(member->SubstituteTemplateArgs(TemplateParams, templateargs, curnamespace, errors));
				}
			}
		}
	}

	return Metadata::EpochType_Error;
}

//
// Perform compile-time code execution tasks for a structure definition
//
bool Structure::CompileTimeCodeExecution(StringHandle myname, Namespace& curnamespace, CompileErrors& errors)
{
	if(IsTemplate())
		return true;

	if(CompileTimeCodeExecuted)
		return true;

	CompileTimeCodeExecuted = true;

	ConstructorName = curnamespace.Functions.CreateOverload(curnamespace.Strings.GetPooledString(myname));
	AnonymousConstructorName = curnamespace.Functions.CreateOverload(curnamespace.Strings.GetPooledString(myname));
	CopyConstructorName = curnamespace.Functions.CreateOverload(curnamespace.Strings.GetPooledString(myname));
	GenerateConstructors(myname, ConstructorName, AnonymousConstructorName, CopyConstructorName, CompileTimeParameterVector(), curnamespace, errors);
	return true;
}

//
// Instantiate a structure template
//
bool Structure::InstantiateTemplate(StringHandle myname, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors)
{
	if(!IsTemplate())
		return false;

	// TODO - check that args matches our template param list; do this for ALL template instantiators

	for(std::vector<std::pair<StringHandle, StructureMember*> >::iterator iter = Members.begin(); iter != Members.end(); ++iter)
		iter->second->SubstituteTemplateArgs(TemplateParams, args, curnamespace, errors);

	Namespace* ns = &curnamespace;
	while(ns->Parent)
		ns = ns->Parent;

	ns->Functions.GenerateStructureFunctions(myname, this);
	// TODO - copy constructors for template instances
	GenerateConstructors(myname, curnamespace.Types.Templates.FindConstructorName(myname), curnamespace.Types.Templates.FindAnonConstructorName(myname), 0, args, curnamespace, errors);
	return true;
}

//
// Add a template parameter to a structure
//
// Should only be used when constructing the IR tree
//
void Structure::AddTemplateParameter(Metadata::EpochTypeID type, StringHandle name)
{
	TemplateParams.push_back(std::make_pair(name, type));
}

//
// Pass template arguments on to a structure member
//
void Structure::SetMemberTemplateArgs(StringHandle membername, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors)
{
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(iter->first == membername)
		{
			// TODO - support templated function-typed members
			StructureMemberVariable* var = dynamic_cast<StructureMemberVariable*>(iter->second);
			if(!var)
				throw InternalException("Wrong kind of structure member");

			var->SetTemplateArgs(args);
			if(!IsTemplate())
				var->MyType = var->SubstituteTemplateArgs(TemplateParams, args, curnamespace, errors);

			return;
		}
	}

	throw InternalException("Invalid structure member");
}



//
// Validate a variable structure member
//
bool StructureMemberVariable::Validate(const Namespace& curnamespace, CompileErrors& errors) const
{
	if(curnamespace.Types.GetTypeByName(MyType) == Metadata::EpochType_Error)
	{
		errors.SetContext(TypeIdentifier);
		errors.SemanticError("Unknown type");
		return false;
	}

	return true;
}

//
// Retrieve the type of a structure member variable
//
Metadata::EpochTypeID StructureMemberVariable::GetEpochType(const Namespace& curnamespace) const
{
	return curnamespace.Types.GetTypeByName(MyType);
}

//
// Given a set of template parameters and arguments, set
// up a structure member variable to use them as needed.
//
StringHandle StructureMemberVariable::SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors) const
{
	if(TemplateArgs.empty())
		return MyType;

	CompileTimeParameterVector myargs(TemplateArgs);

	// Simplify arguments as much as possible first
	for(CompileTimeParameterVector::iterator iter = myargs.begin(); iter != myargs.end(); ++iter)
	{
		for(size_t i = 0; i < params.size(); ++i)
		{
			if(params[i].first == iter->Payload.LiteralStringHandleValue)
				iter->Payload.LiteralStringHandleValue = args[i].Payload.LiteralStringHandleValue;
		}
	}

	// Now modify my name and type as appropriate
	if(curnamespace.Types.Structures.IsStructureTemplate(MyType))
		return curnamespace.Types.Templates.InstantiateStructure(MyType, myargs, errors);
	else if(curnamespace.Types.SumTypes.IsTemplate(MyType))
		return curnamespace.Types.SumTypes.InstantiateTemplate(MyType, myargs, errors);
	//else
	//	throw NotImplementedException("Template parameterization not implemented in this context");

	return MyType;
}


//
// Validate a function reference structure member
//
bool StructureMemberFunctionReference::Validate(const Namespace& curnamespace, CompileErrors& errors) const
{
	bool valid = true;

	if(ReturnTypeName && (curnamespace.Types.GetTypeByName(ReturnTypeName) == Metadata::EpochType_Error))
	{
		valid = false;
		if(ReturnTypeIdentifier)
		{
			errors.SetContext(*ReturnTypeIdentifier);
			errors.SemanticError("Unknown type");
		}
		else
		{
			errors.SetContext(FunctionIdentifier);
			errors.SemanticError("Return type unknown");
		}
	}

	for(std::vector<std::pair<StringHandle, const AST::IdentifierT*> >::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		if(curnamespace.Types.GetTypeByName(iter->first) == Metadata::EpochType_Error)
		{
			valid = false;
			errors.SetContext(*iter->second);
			errors.SemanticError("Unknown type");
		}
	}
	
	return valid;
}

//
// Retrieve the signature of a function-typed structure member
//
FunctionSignature StructureMemberFunctionReference::GetSignature(const Namespace& curnamespace) const
{
	FunctionSignature ret;
	if(ReturnTypeName)
		ret.SetReturnType(curnamespace.Types.GetTypeByName(ReturnTypeName));
	else
		ret.SetReturnType(Metadata::EpochType_Void);

	for(std::vector<std::pair<StringHandle, const AST::IdentifierT*> >::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		ret.AddParameter(L"@@auto", curnamespace.Types.GetTypeByName(iter->first));
	}
	return ret;
}

Metadata::EpochTypeID StructureMemberFunctionReference::GetEpochType(const Namespace& curnamespace) const
{
	return curnamespace.Types.FunctionSignatures.FindEpochType(GetSignature(curnamespace));
}

void StructureMemberFunctionReference::PopulateTypeSpace(Namespace& curnamespace)
{
	curnamespace.Types.FunctionSignatures.AllocateEpochType(GetSignature(curnamespace));
}

