//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for handling the invocation of CUDA functions
//

#include "pch.h"

#include "CUDA Wrapper/FunctionCall.h"
#include "CUDA Wrapper/Module.h"


#define ALIGN_UP(offset, alignment) (offset) = ((offset) + (alignment) - 1) & ~((alignment) - 1)


extern bool CUDAAvailableForExecution;
extern bool CUDALibraryLoaded;


//
// Construct and initialize a function call wrapper
//
FunctionCall::FunctionCall(CUfunction handle)
	: ParamOffset(0),
	  FunctionHandle(handle)
{
}

//
// Add a parameter to the function call
//
void FunctionCall::AddPointerParameter(CUdeviceptr devicepointer)
{
	if(!CUDAAvailableForExecution)
		return;

	void* ptr = reinterpret_cast<void*>(static_cast<size_t>(devicepointer));
	ALIGN_UP(ParamOffset, __alignof(ptr));
	cuParamSetv(FunctionHandle, ParamOffset, &ptr, sizeof(ptr));
	ParamOffset += sizeof(ptr);
}

void FunctionCall::AddNumericParameter(size_t size)
{
	if(!CUDAAvailableForExecution)
		return;

	size_t internalsize = size;
	ALIGN_UP(ParamOffset, __alignof(internalsize));
	cuParamSetv(FunctionHandle, ParamOffset, &internalsize, sizeof(internalsize));
	ParamOffset += sizeof(size_t);
}

//
// Actually perform the function call
//
void FunctionCall::ExecuteNormal()
{
	if(!CUDAAvailableForExecution)
		return;

	cuParamSetSize(FunctionHandle, ParamOffset);
	cuFuncSetBlockShape(FunctionHandle, 1, 1, 1);
	cuLaunch(FunctionHandle);
}

void FunctionCall::ExecuteForLoop(size_t count)
{
	if(!CUDAAvailableForExecution)
		return;

	cuParamSetSize(FunctionHandle, ParamOffset);
	cuFuncSetBlockShape(FunctionHandle, 1, 1, 1);
	cuLaunchGrid(FunctionHandle, static_cast<unsigned>(count), 1);
}

