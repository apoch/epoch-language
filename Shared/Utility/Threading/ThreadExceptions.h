//
// The Epoch Language Project
// Shared Library Code
//
// Exception wrapper classes for the threading/task system
//

#pragma once


// Dependencies
#include "Utility/Exception.h"


namespace Threads
{

	//
	// This exception class wraps the generic failure exception for the threading/task
	// system. These errors are generally not recoverable.
	//
	class ThreadException : public Exception
	{
	// Construction
	public:
		ThreadException(const char* message) : Exception(message) { }
		ThreadException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "A general error has occurred while working with an asynchronous task.";
		}
	};

}


