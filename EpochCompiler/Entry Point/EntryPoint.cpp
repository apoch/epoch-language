//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Library entry/load/unload point
//

#include "pch.h"


//
// Entry/load/unload hook call
//
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
	// We don't do anything interesting here; everything waits until library exports are called.
    return TRUE;
}

