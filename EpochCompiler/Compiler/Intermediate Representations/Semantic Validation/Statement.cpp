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

#include "Libraries/Library.h"


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
	FunctionCompileHelperTable::const_iterator fchiter = program.InfoTable.FunctionHelpers->find(Name);
	if(fchiter != program.InfoTable.FunctionHelpers->end())
		fchiter->second(*this, program, activescope, inreturnexpr);

	return true;
}

