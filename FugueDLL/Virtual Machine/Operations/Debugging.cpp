//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for debugging Epoch programs
//

#include "pch.h"

#include "Virtual Machine/Operations/Debugging.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Types Management/Typecasts.h"

#include "User Interface/Output.h"
#include "User Interface/Input.h"


using namespace VM;
using namespace VM::Operations;



//
// Output an expression (which evaluates to a string) to the debugger interface
//
void DebugWriteStringExpression::ExecuteFast(ExecutionContext& context)
{
	UI::OutputStream output;

	output << UI::lightblue << L"DEBUG: " << UI::resetcolor;
	
	{
		StringVariable value(context.Stack.GetCurrentTopOfStack());
		output << value.GetValue() << std::endl;
	}
	context.Stack.Pop(StringVariable::GetStorageSize());
}

RValuePtr DebugWriteStringExpression::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}


//
// Perform a blocking read from the debugger interface and return the input string
//
RValuePtr DebugReadStaticString::ExecuteAndStoreRValue(ExecutionContext& context)
{
	UI::Input input;
	return RValuePtr(new StringRValue(input.BlockingRead()));
}

void DebugReadStaticString::ExecuteFast(ExecutionContext& context)
{
	UI::Input input;
	input.BlockingRead();
}

