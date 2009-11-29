//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper object for marshalling variable data between the host and the CUDA device
//

#include "pch.h"

#include "CUDA Wrapper/VariableBuffer.h"
#include "CUDA Wrapper/FunctionCall.h"


//
// Construct and initialize a variable marshalling wrapper
//
VariableBuffer::VariableBuffer(const std::list<Traverser::ScopeContents>& variables)
	: Variables(variables)
{
}

//
// Copy host-side variable contents to the CUDA device
//
void VariableBuffer::CopyToDevice(HandleType activatedscopehandle)
{
	SyncBufferForReals.PassVariablesToDevice(Variables, activatedscopehandle);
}

//
// Read back variable contents from the CUDA device
//
void VariableBuffer::CopyFromDevice(HandleType activatedscopehandle)
{
	SyncBufferForReals.RetrieveVariablesFromDevice(Variables, activatedscopehandle);
}


//
// Prepare a function call wrapper for invocation using this block of variable data
//
// The general idea is to pass the CUDA code device-side pointers containing each block
// of data (where blocks are allocated for each necessary data type). Each pointer is
// therefore passed as a function parameter to allow easy access.
//
void VariableBuffer::PrepareFunctionCall(FunctionCall& func)
{
	func.AddParameter(SyncBufferForReals.GetDevicePointer());
}

