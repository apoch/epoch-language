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
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Session.h"
#include "Compiler/CompileErrors.h"


using namespace IRSemantics;


// External prototypes (yeah, I'm lazy)
void CompileConstructorStructure(IRSemantics::Statement& statement, IRSemantics::Program& program, IRSemantics::CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);


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
bool Structure::Validate(const Program& program, CompileErrors& errors) const
{
	if(IsTemplate())
		return true;

	bool valid = true;

	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(!iter->second->Validate(program, errors))
			valid = false;
	}

	return valid;
}


void Structure::GenerateConstructors(StringHandle myname, StringHandle constructorname, StringHandle anonconstructorname, const CompileTimeParameterVector& templateargs, Program& program, CompileErrors& errors) const
{
	// Generate standard constructor
	{
		size_t i = 0;
		FunctionSignature signature;
		signature.AddParameter(L"id", VM::EpochType_Identifier, true);
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
		{
			VM::EpochTypeID paramtype = iter->second->GetEpochType(program);

			if(paramtype == VM::EpochType_Error)
				paramtype = SubstituteTemplateParams(iter->first, templateargs, program);

			signature.AddParameter(program.GetString(iter->first), paramtype, false);
			++i;

			if(iter->second->GetMemberType() == StructureMember::FunctionReference)
			{
				const StructureMemberFunctionReference* funcref = dynamic_cast<const StructureMemberFunctionReference*>(iter->second);
				signature.SetFunctionSignature(i, funcref->GetSignature(program));
			}
		}

		program.Session.FunctionSignatures.insert(std::make_pair(constructorname, signature));
		program.Session.InfoTable.FunctionHelpers->insert(std::make_pair(constructorname, &CompileConstructorStructure));
	}

	// Generate anonymous constructor
	{
		size_t i = 0;
		FunctionSignature signature;
		for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
		{
			VM::EpochTypeID paramtype = iter->second->GetEpochType(program);
			if(paramtype == VM::EpochType_Error)
				paramtype = SubstituteTemplateParams(iter->first, templateargs, program);

			signature.AddParameter(program.GetString(iter->first), paramtype, false);

			if(iter->second->GetMemberType() == StructureMember::FunctionReference)
			{
				const StructureMemberFunctionReference* funcref = dynamic_cast<const StructureMemberFunctionReference*>(iter->second);
				signature.SetFunctionSignature(i, funcref->GetSignature(program));
			}

			++i;
		}
		signature.SetReturnType(program.LookupType(myname));

		program.Session.FunctionSignatures.insert(std::make_pair(anonconstructorname, signature));
	}

	program.Session.FunctionOverloadNames[myname].insert(anonconstructorname);

	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		StringHandle funcname = program.FindStructureMemberAccessOverload(myname, iter->first);

		VM::EpochTypeID type = iter->second->GetEpochType(program);
		if(type == VM::EpochType_Error)
			type = SubstituteTemplateParams(iter->first, templateargs, program);

		std::auto_ptr<Function> func(new Function);
		std::auto_ptr<Expression> retexpr(new Expression);
		retexpr->AddAtom(new ExpressionAtomCopyFromStructure(type, iter->first));
		func->SetReturnExpression(retexpr.release());

		std::auto_ptr<ScopeDescription> scope(new ScopeDescription(program.GetGlobalScope()));
		scope->AddVariable(L"identifier", program.AddString(L"identifier"), program.LookupType(myname), true, VARIABLE_ORIGIN_PARAMETER);
		scope->AddVariable(L"ret", program.AddString(L"ret"), type, false, VARIABLE_ORIGIN_RETURN);
		std::auto_ptr<CodeBlock> codeblock(new CodeBlock(scope.release()));
		program.AllocateLexicalScopeName(codeblock.get());
		func->SetCode(codeblock.release());
		func->SetName(funcname);
		func->AddParameter(program.FindString(L"identifier"), new FunctionParamTyped(type, true), errors);
		func->SuppressReturnRegister();

		program.AddFunction(funcname, funcname, func.release(), errors);
	}
}


VM::EpochTypeID Structure::SubstituteTemplateParams(StringHandle membername, const CompileTimeParameterVector& templateargs, const Program& program) const
{
	if(!IsTemplate())
		return VM::EpochType_Error;

	if(templateargs.size() != TemplateParams.size())
		return VM::EpochType_Error;

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
							return program.LookupType(templateargs[i].Payload.LiteralStringHandleValue);
					}

					return member->GetEpochType(program);
				}
			}
		}
	}

	return VM::EpochType_Error;
}


bool Structure::CompileTimeCodeExecution(StringHandle myname, Program& program, CompileErrors& errors)
{
	if(IsTemplate())
		return true;

	ConstructorName = program.CreateFunctionOverload(program.GetString(myname));
	AnonymousConstructorName = program.CreateFunctionOverload(program.GetString(myname));
	GenerateConstructors(myname, ConstructorName, AnonymousConstructorName, CompileTimeParameterVector(), program, errors);
	return true;
}

bool Structure::InstantiateTemplate(StringHandle myname, const CompileTimeParameterVector& args, Program& program, CompileErrors& errors)
{
	if(!IsTemplate())
		return false;

	// TODO - sanity check (someplace, probably not here) that args matches our template param list

	program.GenerateStructureFunctions(myname, this);
	GenerateConstructors(myname, program.FindTemplateConstructorName(myname), program.FindTemplateAnonConstructorName(myname), args, program, errors);
	return true;
}



//
// Validate a variable structure member
//
bool StructureMemberVariable::Validate(const Program& program, CompileErrors& errors) const
{
	if(program.LookupType(MyType) == VM::EpochType_Error)
	{
		errors.SetContext(TypeIdentifier);
		errors.SemanticError("Unknown type");
		return false;
	}

	return true;
}


VM::EpochTypeID StructureMemberVariable::GetEpochType(const Program& program) const
{
	return program.LookupType(MyType);
}



//
// Validate a function reference structure member
//
bool StructureMemberFunctionReference::Validate(const Program& program, CompileErrors& errors) const
{
	bool valid = true;

	if(ReturnTypeName && (program.LookupType(ReturnTypeName) == VM::EpochType_Error))
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
		if(program.LookupType(iter->first) == VM::EpochType_Error)
		{
			valid = false;
			errors.SetContext(*iter->second);
			errors.SemanticError("Unknown type");
		}
	}
	
	return valid;
}

FunctionSignature StructureMemberFunctionReference::GetSignature(const Program& program) const
{
	FunctionSignature ret;
	if(ReturnTypeName)
		ret.SetReturnType(program.LookupType(ReturnTypeName));
	else
		ret.SetReturnType(VM::EpochType_Void);

	for(std::vector<std::pair<StringHandle, const AST::IdentifierT*> >::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		ret.AddParameter(L"@@auto", program.LookupType(iter->first), false);
	}
	return ret;
}

void Structure::AddTemplateParameter(VM::EpochTypeID type, StringHandle name)
{
	TemplateParams.push_back(std::make_pair(name, type));
}

