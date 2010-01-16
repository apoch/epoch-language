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
void DebugWriteStringExpression::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	UI::OutputStream output;

	output << UI::lightblue << L"DEBUG: " << UI::resetcolor;
	
	{
		StringVariable value(stack.GetCurrentTopOfStack());
		output << value.GetValue() << std::endl;
	}
	stack.Pop(StringVariable::GetStorageSize());
}

RValuePtr DebugWriteStringExpression::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}


//
// Perform a blocking read from the debugger interface and return the input string
//
RValuePtr DebugReadStaticString::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	UI::Input input;
	return RValuePtr(new StringRValue(input.BlockingRead()));
}

void DebugReadStaticString::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	UI::Input input;
	input.BlockingRead();
}

