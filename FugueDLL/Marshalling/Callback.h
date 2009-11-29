//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Marshalling system for external C-style callbacks into Epoch code
//
// WARNING - all marshalling code is platform-specific!
//

#pragma once


// Forward declarations
namespace VM
{
	class Function;
}


namespace Marshalling
{
	void* RequestMarshalledCallback(VM::Function* callbackfunction);

	void Clean();
}

