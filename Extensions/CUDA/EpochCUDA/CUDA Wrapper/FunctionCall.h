//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for handling the invocation of CUDA functions
//

#pragma once


// Dependencies
#include <cuda.h>


// Forward declarations
class Module;


class FunctionCall
{
// Construction
public:
	FunctionCall(CUfunction handle);

// Parameter management
public:
	void AddPointerParameter(CUdeviceptr devicepointer);
	void AddNumericParameter(size_t size);

// Execution interface
public:
	void ExecuteNormal();
	void ExecuteForLoop(size_t count);

// Internal tracking
private:
	CUfunction FunctionHandle;
	unsigned ParamOffset;
};

