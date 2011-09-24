//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a function
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"


using namespace IRSemantics;


//
// Destruct and clean up a function
//
Function::~Function()
{
	for(std::vector<Param>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete iter->Parameter;

	delete Code;
	delete Return;
}

//
// Add a parameter to a function's signature
//
void Function::AddParameter(StringHandle name, FunctionParam* param)
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			delete param;
			throw std::exception("Duplicate function parameter name");		// TODO - better exceptions
		}
	}

	Parameters.push_back(Param(name, param));
}

//
// Set a function's return expression
//
void Function::SetReturnExpression(IRSemantics::Expression* expression)
{
	delete Return;
	Return = expression;
}

//
// Set a function's code body
//
void Function::SetCode(CodeBlock* code)
{
	delete Code;
	Code = code;
}

//
// Validate a function definition
//
bool Function::Validate(const IRSemantics::Program& program) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->Validate(program))
			return false;
	}

	return true;
}


//
// Validate a named function parameter
//
bool FunctionParamNamed::Validate(const IRSemantics::Program& program) const
{
	return (program.LookupType(MyType) != VM::EpochType_Error);
}

//
// Validate a higher order function parameter
//
bool FunctionParamFuncRef::Validate(const IRSemantics::Program& program) const
{
	if(program.LookupType(ReturnType) == VM::EpochType_Error)
		return false;

	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		if(program.LookupType(*iter) == VM::EpochType_Error)
			return false;
	}

	return true;
}

//
// Validate an expression function parameter
//
bool FunctionParamExpression::Validate(const IRSemantics::Program& program) const
{
	if(!MyExpression)
		return false;

	return MyExpression->Validate(program);
}

//
// Destruct and clean up an expression function parameter
//
FunctionParamExpression::~FunctionParamExpression()
{
	delete MyExpression;
}

