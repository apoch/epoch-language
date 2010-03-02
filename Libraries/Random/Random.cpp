//
// The Epoch Language Project
// Auxiliary Libraries
//
// Library for working with random numbers
//

#include "stdlib.h"
#include "windows.h"
#include <vector>

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"

#include "Marshalling/LibraryImporting.h"


//
// Main entry/exit point for the DLL - initialization and cleanup should be done here
//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		// Initialize random number system
		srand(::GetTickCount());
	}

    return TRUE;
}


//
// Callback that is invoked when a program starts running which loads this library.
// Our goal here is to register all of the functions in the library with the Epoch
// virtual machine, so that we can call the library from the program.
//
void __stdcall LinkToEpochVM(RegistrationTable registration, void* bindrecord)
{
	std::vector<ParamData> params;
	params.push_back(ParamData(L"maximum", VM::EpochVariableType_Integer));
	registration.RegisterFunction(L"random", "GetRandomNumber", &params[0], params.size(), VM::EpochVariableType_Integer, VM::EpochVariableType_Error, bindrecord);
}



//-------------------------------------------------------------------------------
// Routines exported by the DLL
//-------------------------------------------------------------------------------

//
// Simple standard C library random number generator
//
Integer32 __stdcall GetRandomNumber(Integer32 maximum)
{
	return static_cast<Integer32>(static_cast<Real>(rand()) / static_cast<Real>(RAND_MAX) * static_cast<Real>(maximum));
}

