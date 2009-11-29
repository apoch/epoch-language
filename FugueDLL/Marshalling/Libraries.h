//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Routines for binding to external libraries
//

#pragma once

// Forward declarations
namespace VM
{
	class Program;
}


namespace Marshalling
{
	void BindToLibrary(const std::wstring& filename, VM::Program& program);
}

