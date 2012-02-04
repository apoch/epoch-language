//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class for representing statements
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/Session.h"


using namespace IRSemantics;


Statement::Statement(StringHandle name)
	: Name(name)
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
		if(!(*iter)->CompileTimeCodeExecution(program, activescope))
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

	return true;
}


bool PreOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	// TODO - type inference on preops
	return true;
}

bool PostOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	// TODO - type inference on postops
	return true;
}

