//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Exception wrapper classes for the parser subsystem
//

#pragma once


// Dependencies
#include "Utility/Exception.h"


namespace Parser
{

	//
	// This exception represents an error in the parser's internal state, usually
	// due to a bug in the parser itself. Users should not in general be able to
	// trigger these exceptions by writing malformed code.
	//
	class ParserFailureException : public Exception
	{
	// Construction
	public:
		ParserFailureException(const char* message) : Exception(message) { }
		ParserFailureException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "An internal error has occurred while parsing the Epoch program. "
				"This likely represents a bug in the parser itself.";
		}
	};


	//
	// This exception is thrown when a user-caused syntax error occurs. These are
	// used heavily in the parser to denote unrecoverable errors. However, if there
	// is a chance that parsing could continue as normal after the error, it is
	// better to use the ReportFatalError() function. This allows multiple errors
	// to be detected in a single parser pass, rather than terminating the parse
	// after the first error occurs.
	//
	class SyntaxException : public Exception
	{
	// Construction
	public:
		SyntaxException(const char* message) : Exception(message) { }
		SyntaxException(const std::string& message) : Exception(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "The Epoch source code contains an error.";
		}
	};

}


