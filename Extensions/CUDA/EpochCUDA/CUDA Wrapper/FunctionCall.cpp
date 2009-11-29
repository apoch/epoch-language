//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for handling the invocation of CUDA functions
//

#include "pch.h"

#include "CUDA Wrapper/FunctionCall.h"
#include "CUDA Wrapper/Module.h"


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
void FunctionCall::AddParameter(CUdeviceptr devicepointer)
{
	#define ALIGN_UP(offset, alignment) (offset) = ((offset) + (alignment) - 1) & ~((alignment) - 1)

	void* ptr = reinterpret_cast<void*>(static_cast<size_t>(devicepointer));
	ALIGN_UP(ParamOffset, __alignof(ptr));
	cuParamSetv(FunctionHandle, ParamOffset, &ptr, sizeof(ptr));
	ParamOffset += sizeof(ptr);
}

//
// Actually perform the function call
//
void FunctionCall::Execute()
{
	cuParamSetSize(FunctionHandle, ParamOffset);
	cuLaunch(FunctionHandle);
}

