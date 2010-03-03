//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Entry point for the interop DLL
//

#include "pch.h"

#include "CUDA Wrapper/Initialization.h"

#include "Code Generation/CompiledCodeManager.h"

#include "Configuration/ConfigFile.h"

#include "Utility/Files/TempFile.h"
#include "Utility/Strings.h"

#include "FugueVMAccess.h"


// Global access to configuration data; placed here for lack of a better place
Config::ConfigReader Configuration;



//
// Entry point routine; handles initialization and cleanup of the library
//
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
	try
	{
		if(reason == DLL_PROCESS_DETACH)
		{
			ShutdownCUDA();
			Compiler::DestroyTempFiles();
			TemporaryFileWriter::RemoveAllFiles();
		}

		return TRUE;
	}
	catch(std::exception& e)
	{
		if(reason == DLL_PROCESS_DETACH)
		{
			::MessageBoxA(0, e.what(), "CUDA Layer for Epoch", MB_ICONSTOP);
			return TRUE;
		}
			
		FugueVMAccess::Interface.Error(widen(e.what()).c_str());
	}
	catch(...)
	{
		if(reason == DLL_PROCESS_DETACH)
		{
			::MessageBox(0, L"Unrecognized exception thrown during DLL detach; something is probably horribly wrong.", L"CUDA Layer for Epoch", MB_ICONSTOP);
			return TRUE;
		}

		FugueVMAccess::Interface.Error(L"Unrecognized exception thrown");
	}

	return FALSE;
}


