//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Exception wrapper classes for the bytecode subsystem
//

#pragma once


// Dependencies
#include "Utility/Exception.h"


//
// This exception indicates corrupt or unrecognizable data in a bytecode stream.
//
class InvalidBytecodeException : public Exception
{
// Construction
public:
	InvalidBytecodeException(const char* message) : Exception(message) { }
	InvalidBytecodeException(const std::string& message) : Exception(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "Epoch bytecode is corrupted, from a different version of the Epoch subsystems, or otherwise not recognized.";
	}
};

