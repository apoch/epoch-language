//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Exception wrapper classes for the virtual machine
//

#pragma once


// Dependencies
#include "Utility/Exception.h"


namespace VM
{

	//
	// This exception denotes a failure due to usage of a feature that is not yet implemented in the VM.
	// Generally these can't be recovered from due to the fact that skipping the operation would leave
	// the VM in an unpredictable state.
	//
	class NotImplementedException : public Exception
	{
	// Construction
	public:
		NotImplementedException(const char* message) : Exception(message) { }
		NotImplementedException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "Support for an Epoch language feature is not complete; "
				"in most cases the code is correct but the VM is missing "
				"the implementation. Please report this error and provide "
				"any relevant code.";
		}
	};


	//
	// This exception represents failures which should technically never occur unless something
	// breaks the internals of the VM. These errors are not recoverable.
	//
	class InternalFailureException : public Exception
	{
	// Construction
	public:
		InternalFailureException(const char* message) : Exception(message) { }
		InternalFailureException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "An internal failure has occured in the Epoch virtual machine. "
				"This type of error generally occurs when the internal state of the "
				"VM is corrupted. Please report this error along with any source "
				"code that reproduces the issue.";
		}
	};


	//
	// This exception is used for cases where the program being executed has caused an error.
	// Basically this means the programmer did something evil.
	//
	class ExecutionException : public Exception
	{
	// Construction
	public:
		ExecutionException(const char* message) : Exception(message) { }
		ExecutionException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "The Epoch program being executed has performed an illegal operation.";
		}
	};


	//
	// This exception indicates that an identifier was expected to be bound to a variable,
	// but no binding information was found in any active scope. These errors are never
	// recoverable.
	//
	class MissingVariableException : public Exception
	{
	// Construction
	public:
		MissingVariableException(const char* message) : Exception(message) { }
		MissingVariableException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "Variable name not recognized or is not available in this scope.";
		}
	};

	//
	// If an identifier shadows an identifier at a higher scope, or within the current
	// scope itself, this exception is thrown. Most of the time this should occur only
	// during parsing, but there are also runtime cases (via metaprogramming) that can
	// throw the exception as well.
	//
	class DuplicateIdentifierException : public Exception
	{
	// Construction
	public:
		DuplicateIdentifierException(const char* message) : Exception(message) { }
		DuplicateIdentifierException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "All identifiers in Epoch must be unique within their containing scope.";
		}
	};

}


