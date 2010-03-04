//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Central one-time initialization of the CUDA drivers
//


#include "pch.h"

#include "CUDA Wrapper/Module.h"
#include "Code Generation/EASMToCUDA.h"

#include <cuda.h>


// We create a single context per process; this variable holds the handle to that context
CUcontext ContextHandle = 0;


//
// Invoke the CUDA driver's initialization logic
//
bool InitializeCUDA()
{
	// We delay-load the CUDA DLL so we have a chance to trap any failures
	// and respond by deactivating the language extension. All delay-load
	// failures come in the form of SEH exceptions, so we wrap our first
	// call into the DLL with a SEH handler to ensure the process doesn't
	// get nuked if anything goes wrong.

	__try
	{
		if(cuInit(0) != CUDA_SUCCESS)
			return false;

		CUdevice devicehandle;

		int devicecount = 0;
		cuDeviceGetCount(&devicecount);
		if(devicecount <= 0)
			return false;

		if(cuDeviceGet(&devicehandle, 0) != CUDA_SUCCESS)
			return false;

		if(cuCtxCreate(&ContextHandle, CU_CTX_MAP_HOST, devicehandle) != CUDA_SUCCESS)
			return false;

		CUDALibraryLoaded = true;
		CUDAAvailableForExecution = true;
		return true;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}


//
// Shut down and clean up the CUDA driver
//
void ShutdownCUDA()
{
	Module::ReleaseAllModules();
	if(CUDALibraryLoaded)
		cuCtxDestroy(ContextHandle);
}

