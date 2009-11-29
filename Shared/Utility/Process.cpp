//
// The Epoch Language Project
// Shared Library Code
//
// Helper routines for spawning external processes
//

#include "pch.h"

#include "Utility/Process.h"



//
// Spin off a process and wait until it completes before returning
//
unsigned LaunchProcessSynchronous(const std::wstring& executablefile, const std::wstring& parameters)
{
	std::vector<WCHAR> buffer(std::max(static_cast<size_t>(2048), parameters.length() + 2), L'\0');
	std::copy(parameters.begin(), parameters.end(), buffer.begin());


	PROCESS_INFORMATION procinfo;
	STARTUPINFO startinfo;

	::ZeroMemory(&procinfo, sizeof(PROCESS_INFORMATION));
	::ZeroMemory(&startinfo, sizeof(STARTUPINFO));

	startinfo.cb = sizeof(STARTUPINFO);

	if(!::CreateProcess(executablefile.c_str(), &buffer[0], NULL, NULL, FALSE, 0, NULL, NULL, &startinfo, &procinfo))
		throw ProcessLaunchException("Could not locate the requested executable file (CreateProcess failed)");

	DWORD exitcode;

	::WaitForSingleObject(procinfo.hProcess, INFINITE);
	::GetExitCodeProcess(procinfo.hProcess, &exitcode);

	::CloseHandle(procinfo.hThread);
	::CloseHandle(procinfo.hProcess);

	return exitcode;
}

