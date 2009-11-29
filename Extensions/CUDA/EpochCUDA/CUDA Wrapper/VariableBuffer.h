//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper object for marshalling variable data between the host and the CUDA device
//

#pragma once


// Dependencies
#include <cuda.h>

#include "Traverser/TraversalInterface.h"
#include "FugueVMAccess.h"


// Forward declarations
class FunctionCall;


//
// Helper class for storing data buffers of specific types; used internally by the VariableBuffer
//
template <typename T, VM::EpochVariableTypeID DataType>
class SynchronizableBuffer
{
// Construction
public:
	SynchronizableBuffer()
		: DevicePointer(0)
	{ }

// Data transfer operations
public:
	void PassVariablesToDevice(const std::list<Traverser::ScopeContents>& variables, HandleType activatedscopehandle)
	{
		InternalBuffer.clear();

		for(std::list<Traverser::ScopeContents>::const_iterator iter = variables.begin(); iter != variables.end(); ++iter)
		{
			if(iter->Type == DataType)
			{
				Traverser::Payload payload;
				FugueVMAccess::Interface.MarshalRead(activatedscopehandle, iter->Identifier, &payload);
				InternalBuffer.push_back(payload.GetValueByType<T>());
			}
		}

		if(DevicePointer)
		{
			cuMemFree(DevicePointer);
			DevicePointer = 0;
		}

		unsigned int buffersizeinbytes = static_cast<unsigned int>(sizeof(T) * InternalBuffer.size());
		cuMemAlloc(&DevicePointer, buffersizeinbytes);
		cuMemcpyHtoD(DevicePointer, &InternalBuffer[0], buffersizeinbytes);
	}

	void RetrieveVariablesFromDevice(const std::list<Traverser::ScopeContents>& variables, HandleType activatedscopehandle)
	{
		if(InternalBuffer.empty())
			return;

		unsigned int buffersizeinbytes = static_cast<unsigned int>(sizeof(T) * InternalBuffer.size());
		cuMemcpyDtoH(&InternalBuffer[0], DevicePointer, buffersizeinbytes);

		unsigned index = 0;

		for(std::list<Traverser::ScopeContents>::const_iterator iter = variables.begin(); iter != variables.end(); ++iter)
		{
			Traverser::Payload payload;
			payload.Type = iter->Type;
			payload.SetValue(InternalBuffer[index++]);
			FugueVMAccess::Interface.MarshalWrite(activatedscopehandle, iter->Identifier, &payload);
		}

		cuMemFree(DevicePointer);
		DevicePointer = 0;
	}

// Additional accessors
public:
	CUdeviceptr GetDevicePointer() const
	{ return DevicePointer; }

// Internal tracking
private:
	CUdeviceptr DevicePointer;
	std::vector<T> InternalBuffer;
};


//
// Wrapper class for batch-copying variable data back and forth to the CUDA device
//
class VariableBuffer
{
// Construction
public:
	VariableBuffer(const std::list<Traverser::ScopeContents>& variables);

// Data copy operations
public:
	void CopyToDevice(HandleType activatedscopehandle);
	void CopyFromDevice(HandleType activatedscopehandle);

// Helpers for function calls
public:
	void PrepareFunctionCall(FunctionCall& func);

// Internal tracking
private:
	const std::list<Traverser::ScopeContents>& Variables;
	SynchronizableBuffer<Real, VM::EpochVariableType_Real> SyncBufferForReals;
};

