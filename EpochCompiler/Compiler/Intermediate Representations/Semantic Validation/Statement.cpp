//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class for representing statements
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"


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

