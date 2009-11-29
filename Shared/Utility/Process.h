//
// The Epoch Language Project
// Shared Library Code
//
// Helper routines for spawning external processes
//

#pragma once


//
// Special exception class so that users of the routines can
// detect specific process-related errors and handle them
// individually rather than as vanilla std::exceptions.
//
class ProcessLaunchException : public std::exception
{
// Construction
public:
	ProcessLaunchException(const char* message)
		: exception(message)
	{ }
};


unsigned LaunchProcessSynchronous(const std::wstring& executablefile, const std::wstring& parameters);

