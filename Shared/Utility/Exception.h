//
// The Epoch Language Project
// Shared Library Code
//
// Basic exception wrapper class.
//

#pragma once


// Dependencies
#include <exception>
#include <string>


//
// Base exception class used for internal exception handling
//
class Exception : public std::exception
{
// Construction
public:
	Exception(const char* message)
		: exception(message)
	{
	}

	Exception(const std::string& message)
		: exception(message.c_str())
	{
	}

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const = 0;
};


//
// Fatal exception class; these should in general only be caught if it
// is necessary to use the catch to clean something up, and then bail.
// In other words, a thrown FatalException should always terminate the
// current process sooner or later, although note that the program may
// elect to catch the exception and restart a driver loop or something
// similar in order to continue execution.
//
class FatalException : public Exception
{
// Construction
public:
	FatalException(const char* message) : Exception(message) { }
	FatalException(const std::string& message) : Exception(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The Epoch language subsystem has encountered an unrecoverable error.";
	}
};


//
// Recoverable exception wrapper
//
class RecoverableException : public Exception
{
// Construction
public:
	RecoverableException(const char* message) : Exception(message) { }
	RecoverableException(const std::string& message) : Exception(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The Epoch language subsystem has encountered an error; this situation should have been recoverable but for some reason was not handled.";
	}
};


//
// Exception specifically for memory related errors
//
class MemoryException : public FatalException
{
// Construction
public:
	MemoryException(const char* message) : FatalException(message) { }
	MemoryException(const std::string& message) : FatalException(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The Epoch memory manager has encountered an error.";
	}
};


//
// Exception specifically for file IO errors
//
class FileException : public RecoverableException
{
// Construction
public:
	FileException(const char* message) : RecoverableException(message) { }
	FileException(const std::string& message) : RecoverableException(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The Epoch subsystem encountered a file I/O error.";
	}
};


//
// Special exception for reporting features which are not yet implemented
//
class NotImplementedException : public FatalException
{
// Construction
public:
	NotImplementedException(const char* message) : FatalException(message) { }
	NotImplementedException(const std::string& message) : FatalException(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The Epoch subsystem attempted to make use of a feature which has not yet been implemented.";
	}
};


//
// Exception for signalling that an identifier is bogus in some way.
//
class InvalidIdentifierException : public RecoverableException
{
// Construction
public:
	InvalidIdentifierException(const char* message) : RecoverableException(message) { }
	InvalidIdentifierException(const std::string& message) : RecoverableException(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "A program identifier is invalid or inactive in the current scope.";
	}
};


//
// Exception indicating that something is wrong with the way the Epoch system was compiled
//
class CompileSettingsException : public FatalException
{
// Construction
public:
	CompileSettingsException(const char* message) : FatalException(message) { }
	CompileSettingsException(const std::string& message) : FatalException(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The Epoch subsystem has been compiled incorrectly or in an inconsistent manner.";
	}
};

