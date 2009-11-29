//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Main entry point for the assembler DLL
//

#include "pch.h"


//
// Initialize the utility
//
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
#ifdef _DEBUG
		// First things first: debug memory leaks
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);

		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	}
	
	return TRUE;
}
