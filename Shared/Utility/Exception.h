//
// The Epoch Language Project
// Shared Library Code
//
// Basic std::runtime_error wrapper class.
//

#pragma once


// Dependencies
#include <stdexcept>
#include <string>


//
// Base std::runtime_error class used for internal std::runtime_error handling
//
class Exception : public std::runtime_error
{
// Construction
public:
	explicit Exception(const char* message)
		: std::runtime_error(message)
	{
	}

	explicit Exception(const std::string& message)
		: std::runtime_error(message.c_str())
	{
	}
};


//
// Fatal std::runtime_error class; these should in general only be caught if it
// is necessary to use the catch to clean something up, and then bail.
// In other words, a thrown FatalException should always terminate the
// current process sooner or later, although note that the program may
// elect to catch the std::runtime_error and restart a driver loop or something
// similar in order to continue execution.
//
class FatalException : public Exception
{
// Construction
public:
	explicit FatalException(const char* message) : Exception(message) { }
	explicit FatalException(const std::string& message) : Exception(message) { }
};


//
// Recoverable std::runtime_error wrapper
//
class RecoverableException : public Exception
{
// Construction
public:
	explicit RecoverableException(const char* message) : Exception(message) { }
	explicit RecoverableException(const std::string& message) : Exception(message) { }
};


//
// Exception specifically for memory related errors
//
class MemoryException : public FatalException
{
// Construction
public:
	explicit MemoryException(const char* message) : FatalException(message) { }
	explicit MemoryException(const std::string& message) : FatalException(message) { }
};


//
// Exception specifically for file IO errors
//
class FileException : public RecoverableException
{
// Construction
public:
	explicit FileException(const char* message) : RecoverableException(message) { }
	explicit FileException(const std::string& message) : RecoverableException(message) { }
};


//
// Special std::runtime_error for reporting features which are not yet implemented
//
class NotImplementedException : public FatalException
{
// Construction
public:
	explicit NotImplementedException(const char* message) : FatalException(message) { }
	explicit NotImplementedException(const std::string& message) : FatalException(message) { }
};


//
// Exception for signalling that an identifier is bogus in some way.
//
class InvalidIdentifierException : public RecoverableException
{
// Construction
public:
	explicit InvalidIdentifierException(const char* message) : RecoverableException(message) { }
	explicit InvalidIdentifierException(const std::string& message) : RecoverableException(message) { }
};


//
// Exception indicating that something is wrong with the way the Epoch system was compiled
//
class CompileSettingsException : public FatalException
{
// Construction
public:
	explicit CompileSettingsException(const char* message) : FatalException(message) { }
	explicit CompileSettingsException(const std::string& message) : FatalException(message) { }
};

