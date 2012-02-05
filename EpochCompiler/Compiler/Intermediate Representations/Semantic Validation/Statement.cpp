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

#include "Compiler/Session.h"


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

	// TODO - validate overloads etc.

	return valid;
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

bool Statement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	InferenceContext newcontext(Name, InferenceContext::CONTEXT_STATEMENT);

	switch(context.State)
	{
	case InferenceContext::CONTEXT_CODE_BLOCK:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(Name));
		break;

	case InferenceContext::CONTEXT_STATEMENT:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(context.ContextName));
		break;

	case InferenceContext::CONTEXT_EXPRESSION:
	case InferenceContext::CONTEXT_FUNCTION_RETURN:
		// TODO - this is broken, evaluate the actual operators involved and use them w/ overload resolution
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(Name));
		break;

	default:
		throw std::exception("Invalid inference context");				// TODO - better exceptions
	}

	size_t i = 0;
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, newcontext, i))
			return false;

		++i;
	}

	// TODO - handle overloading
	if(context.State == InferenceContext::CONTEXT_FUNCTION_RETURN)
		MyType = program.LookupType(Name);
	else
	{
		if(program.HasFunction(Name))
		{
			InferenceContext funccontext(0, InferenceContext::CONTEXT_GLOBAL);
			Function* func = program.GetFunctions().find(Name)->second;
			func->TypeInference(program, funccontext);
			MyType = func->GetReturnType(program);
		}
		else
		{
			OverloadMap::const_iterator ovmapiter = program.Session.FunctionOverloadNames.find(Name);
			if(ovmapiter != program.Session.FunctionOverloadNames.end())
				MyType = program.Session.FunctionSignatures.find(*ovmapiter->second.begin())->second.GetReturnType();
			else
				MyType = program.Session.FunctionSignatures.find(Name)->second.GetReturnType();
		}
	}

	return true;
}


namespace
{
	VM::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Program& program, const CodeBlock& activescope)
	{
		if(accesslist.empty())
			return VM::EpochType_Error;

		std::vector<StringHandle>::const_iterator iter = accesslist.begin();
		VM::EpochTypeID thetype = activescope.GetScope()->GetVariableTypeByID(*iter);

		while(++iter != accesslist.end())
		{
			StringHandle structurename = program.GetNameOfStructureType(thetype);
			StringHandle memberaccessname = program.FindStructureMemberAccessOverload(structurename, *iter);
			
			thetype = program.GetStructureMemberType(structurename, memberaccessname);
		}

		return thetype;
	}
}


bool PreOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	MyType = InferMemberAccessType(Operand, program, activescope);
	return (MyType != VM::EpochType_Error);
}

bool PostOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	MyType = InferMemberAccessType(Operand, program, activescope);
	return (MyType != VM::EpochType_Error);
}

