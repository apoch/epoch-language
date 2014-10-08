//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Library entry/load/unload point
//

#include "pch.h"


//
// Entry/load/unload hook call
//
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
	// Nothing interesting to do here.

	/*
#ifdef _DEBUG
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags |= _CRTDBG_ALLOC_MEM_DF;
	flags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(flags);

	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
	*/

    return TRUE;
}

