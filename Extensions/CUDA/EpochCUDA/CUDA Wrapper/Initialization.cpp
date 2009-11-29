//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Central one-time initialization of the CUDA drivers
//


#include "pch.h"

#include "CUDA Wrapper/Module.h"

#include <cuda.h>


// We create a single context per process; this variable holds the handle to that context
CUcontext ContextHandle = 0;


//
// Invoke the CUDA driver's initialization logic
//
bool InitializeCUDA()
{
	if(cuInit(0) != CUDA_SUCCESS)
		return false;

	CUdevice devicehandle;

	int devicecount = 0;
	cuDeviceGetCount(&devicecount);
	if(devicecount <= 0)
		throw std::exception("No devices supporting CUDA are available.");

	if(cuDeviceGet(&devicehandle, 0) != CUDA_SUCCESS)
		throw std::exception("Failed to acquire a handle to a CUDA-enabled device (tried slot 0)");

	if(cuCtxCreate(&ContextHandle, CU_CTX_MAP_HOST, devicehandle) != CUDA_SUCCESS)
		throw std::exception("Failed to create a CUDA context with the current device");

	return true;
}


//
// Shut down and clean up the CUDA driver
//
void ShutdownCUDA()
{
	Module::ReleaseAllModules();
	cuCtxDestroy(ContextHandle);
}

