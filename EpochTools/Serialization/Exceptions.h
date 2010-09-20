//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Special exception classes for the serialization subsystem
//

#pragma once


namespace Serialization
{

	//
	// General exception for internal serialization failures. Should only be used
	// for things that represent corrupt (or bogus) code from the parser/compiler
	// chain; anything that might result from a legitimate mistake on the part of
	// the end user should use a different exception class.
	//
	class SerializationException : public FatalException
	{
	// Construction
	public:
		explicit SerializationException(const char* message) : FatalException(message) { }
		explicit SerializationException(const std::string& message) : FatalException(message) { }

	// Helpers for making error reporting more friendly
	public:
		virtual const char* GetErrorPrologue() const
		{
			return "Serialization of compiled code failed due to an internal error.";
		}
	};

}


