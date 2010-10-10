//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Special exception classes for DLL access wrappers
//

#pragma once


namespace DLLAccess
{

	//
	// Exception specifying an error loading a DLL. This may be due to a missing or corrupt
	// file, or an incorrect interface (e.g. if a version mismatch occurs). In general this
	// exception should terminate the program since there's not much we can do about it.
	//
	class DLLException : public FatalException
	{
	// Construction
	public:
		explicit DLLException(const char* message) : FatalException(message) { }
		explicit DLLException(const std::string& message) : FatalException(message) { }
	};

}

