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


// External prototypes (yeah, I'm lazy)
void CompileConstructorStructure(IRSemantics::Statement& statement, IRSemantics::Namespace& curnamespace, IRSemantics::CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);


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


void Structure::GenerateConstructors(StringHandle myname, StringHandle constructorname, StringHandle anonconstructorname, const CompileTimeParameterVector& templateargs, Namespace& curnamespace, CompileErrors& errors) const
{
	// Generate standard constructor
	{
		size_t i = 0;
		FunctionSignature signature;
		signature.AddParameter(L"id", Metadata::EpochType_Identifier, true);
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
		{
			Metadata::EpochTypeID paramtype = iter->second->GetEpochType(curnamespace);

			if(paramtype == Metadata::EpochType_Error)
				paramtype = SubstituteTemplateParams(iter->first, templateargs, curnamespace);

			signature.AddParameter(curnamespace.Strings.GetPooledString(iter->first), paramtype, false);
			++i;

			if(iter->second->GetMemberType() == StructureMember::FunctionReference)
			{
				const StructureMemberFunctionReference* funcref = dynamic_cast<const StructureMemberFunctionReference*>(iter->second);
				signature.SetFunctionSignature(i, funcref->GetSignature(curnamespace));
			}
		}

		curnamespace.Functions.SetSignature(constructorname, signature);
		curnamespace.Functions.SetCompileHelper(constructorname, &CompileConstructorStructure);
	}

	// Generate anonymous constructor
	{
		size_t i = 0;
		FunctionSignature signature;
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
		{
			Metadata::EpochTypeID paramtype = iter->second->GetEpochType(curnamespace);
			if(paramtype == Metadata::EpochType_Error)
				paramtype = SubstituteTemplateParams(iter->first, templateargs, curnamespace);

			signature.AddParameter(curnamespace.Strings.GetPooledString(iter->first), paramtype, false);

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

	curnamespace.Functions.AddOverload(myname, anonconstructorname);

	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		StringHandle funcname = curnamespace.Functions.FindStructureMemberAccessOverload(myname, iter->first);

		Metadata::EpochTypeID type = iter->second->GetEpochType(curnamespace);
		if(type == Metadata::EpochType_Error)
			type = SubstituteTemplateParams(iter->first, templateargs, curnamespace);

		std::auto_ptr<Function> func(new Function);
		std::auto_ptr<Expression> retexpr(new Expression);
		retexpr->AddAtom(new ExpressionAtomCopyFromStructure(type, iter->first));
		func->SetReturnExpression(retexpr.release());

		std::auto_ptr<ScopeDescription> scope(new ScopeDescription(curnamespace.GetGlobalScope()));
		scope->AddVariable(L"identifier", curnamespace.Strings.Pool(L"identifier"), myname, curnamespace.Types.GetTypeByName(myname), true, VARIABLE_ORIGIN_PARAMETER);
		scope->AddVariable(L"ret", curnamespace.Strings.Pool(L"ret"), curnamespace.Types.GetNameOfType(type), type, false, VARIABLE_ORIGIN_RETURN);
		std::auto_ptr<CodeBlock> codeblock(new CodeBlock(scope.release()));
		curnamespace.AllocateLexicalScopeName(codeblock->GetScope());
		func->SetCode(codeblock.release());
		func->SetName(funcname);
		func->AddParameter(curnamespace.Strings.Find(L"identifier"), new FunctionParamTyped(type, true), errors);
		func->SuppressReturnRegister();

		curnamespace.Functions.Add(funcname, funcname, func.release());
	}
}


Metadata::EpochTypeID Structure::SubstituteTemplateParams(StringHandle membername, const CompileTimeParameterVector& templateargs, const Namespace& curnamespace) const
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
						if(member->GetNameOfType() == TemplateParams[i].first)
							return curnamespace.Types.GetTypeByName(templateargs[i].Payload.LiteralStringHandleValue);
					}

					return member->GetEpochType(curnamespace);
				}
			}
		}
	}

	return Metadata::EpochType_Error;
}


bool Structure::CompileTimeCodeExecution(StringHandle myname, Namespace& curnamespace, CompileErrors& errors)
{
	if(IsTemplate())
		return true;

	ConstructorName = curnamespace.Functions.CreateOverload(curnamespace.Strings.GetPooledString(myname));
	AnonymousConstructorName = curnamespace.Functions.CreateOverload(curnamespace.Strings.GetPooledString(myname));
	GenerateConstructors(myname, ConstructorName, AnonymousConstructorName, CompileTimeParameterVector(), curnamespace, errors);
	return true;
}

bool Structure::InstantiateTemplate(StringHandle myname, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors)
{
	if(!IsTemplate())
		return false;

	// TODO - check that args matches our template param list; do this for ALL template instantiators

	for(std::vector<std::pair<StringHandle, StructureMember*> >::iterator iter = Members.begin(); iter != Members.end(); ++iter)
		iter->second->SubstituteTemplateArgs(TemplateParams, args, curnamespace);

	curnamespace.Functions.GenerateStructureFunctions(myname, this);
	GenerateConstructors(myname, curnamespace.Types.Templates.FindConstructorName(myname), curnamespace.Types.Templates.FindAnonConstructorName(myname), args, curnamespace, errors);
	return true;
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


Metadata::EpochTypeID StructureMemberVariable::GetEpochType(const Namespace& curnamespace) const
{
	return curnamespace.Types.GetTypeByName(MyType);
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

FunctionSignature StructureMemberFunctionReference::GetSignature(const Namespace& curnamespace) const
{
	FunctionSignature ret;
	if(ReturnTypeName)
		ret.SetReturnType(curnamespace.Types.GetTypeByName(ReturnTypeName));
	else
		ret.SetReturnType(Metadata::EpochType_Void);

	for(std::vector<std::pair<StringHandle, const AST::IdentifierT*> >::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		ret.AddParameter(L"@@auto", curnamespace.Types.GetTypeByName(iter->first), false);
	}
	return ret;
}

void Structure::AddTemplateParameter(Metadata::EpochTypeID type, StringHandle name)
{
	TemplateParams.push_back(std::make_pair(name, type));
}


void Structure::SetMemberTemplateArgs(StringHandle membername, const CompileTimeParameterVector& args)
{
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(iter->first == membername)
		{
			StructureMemberVariable* var = dynamic_cast<StructureMemberVariable*>(iter->second);
			if(!var)
				throw InternalException("Wrong kind of structure member");

			var->SetTemplateArgs(args);
			return;
		}
	}

	throw InternalException("Invalid structure member");
}


void StructureMemberVariable::SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace& curnamespace)
{
	if(TemplateArgs.empty())
		return;

	// Simplify arguments as much as possible first
	for(CompileTimeParameterVector::iterator iter = TemplateArgs.begin(); iter != TemplateArgs.end(); ++iter)
	{
		for(size_t i = 0; i < params.size(); ++i)
		{
			if(params[i].first == iter->Payload.LiteralStringHandleValue)
				iter->Payload.LiteralStringHandleValue = args[i].Payload.LiteralStringHandleValue;
		}
	}

	// Now modify my name and type as appropriate
	if(curnamespace.Types.Structures.IsStructureTemplate(MyType))
		MyType = curnamespace.Types.Templates.InstantiateStructure(MyType, TemplateArgs);
	else if(curnamespace.Types.SumTypes.IsTemplate(MyType))
		MyType = curnamespace.Types.SumTypes.InstantiateTemplate(MyType, TemplateArgs);
	else
		throw NotImplementedException("Template parameterization not implemented in this context");
}

