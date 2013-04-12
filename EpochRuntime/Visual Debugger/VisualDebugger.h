//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Visual debugger access interface
//

#pragma once


// Forward declarations
namespace VM
{
	class VirtualMachine;
}


namespace VisualDebugger
{
	void ForkDebuggerThread(VM::VirtualMachine* vm);
}

