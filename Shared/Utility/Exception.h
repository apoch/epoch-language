//
// The Epoch Language Project
// Shared Library Code
//
// Basic exception wrapper class.
//

#pragma once


//
// Base exception class used for internal exception handling
//
class Exception : public std::exception
{
// Construction
public:
	Exception(const char* message)
		: exception(message)
	{ }

	Exception(const std::string& message)
		: exception(message.c_str())
	{ }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "A general error has occurred in the Epoch language subsystem.";
	}
};


//
// Exception specifically for memory related errors
//
class MemoryException : public Exception
{
// Construction
public:
	MemoryException(const char* message) : Exception(message) { }
	MemoryException(const std::string& message) : Exception(message) { }

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
class FileException : public Exception
{
// Construction
public:
	FileException(const char* message) : Exception(message) { }
	FileException(const std::string& message) : Exception(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The Epoch subsystem encountered a file I/O error.";
	}
};

