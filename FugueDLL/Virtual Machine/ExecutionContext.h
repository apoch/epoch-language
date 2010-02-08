//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper structure for passing around contextual information
//

#pragma once


// Forward declarations
class StackSpace;


namespace VM
{

	// Forward declarations
	class ActivatedScope;
	class Program;


	enum FlowControlResult
	{
		FLOWCONTROL_NORMAL,					// Proceed with execution as normal
		FLOWCONTROL_BREAK,					// Break from the current loop
		FLOWCONTROL_EXITELSEIFWRAPPER,		// Shortcut out of an if/elseif/else chain
		FLOWCONTROL_RETURN					// Return from the current function
	};


	struct ExecutionContext
	{
	// Construction
	public:
		ExecutionContext(Program& program, ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
			: Scope(scope),
			  Stack(stack),
			  FlowResult(flowresult),
			  RunningProgram(program)
		{ }

	// Data members
	public:
		ActivatedScope& Scope;
		StackSpace& Stack;
		FlowControlResult& FlowResult;
		Program& RunningProgram;
	};

}

