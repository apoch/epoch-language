//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper objects for marshalling variable data between the host and the CUDA device
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

		if(!InternalBuffer.empty())
		{
			unsigned int buffersizeinbytes = static_cast<unsigned int>(sizeof(T) * InternalBuffer.size());
			cuMemAlloc(&DevicePointer, buffersizeinbytes);
			cuMemcpyHtoD(DevicePointer, &InternalBuffer[0], buffersizeinbytes);
		}
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
			if(iter->Type == DataType)
			{
				Traverser::Payload payload;
				payload.Type = iter->Type;
				payload.SetValue(InternalBuffer[index++]);
				FugueVMAccess::Interface.MarshalWrite(activatedscopehandle, iter->Identifier, &payload);
			}
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
// Helper class for storing data buffers of specific array types; used internally by the VariableBuffer
//
template <typename T, VM::EpochVariableTypeID DataType>
class SynchronizableArrayBuffer
{
// Construction
public:
	SynchronizableArrayBuffer()
		: DevicePointer(0), SizesDevicePointer(0)
	{ }

// Preparation operations
public:
	void PrepareArraySizesBuffer()
	{
		if(SizesDevicePointer)
		{
			cuMemFree(SizesDevicePointer);
			SizesDevicePointer = 0;
		}

		if(!InternalBuffer.empty())
		{
			unsigned sizeinbytes = static_cast<unsigned>(sizeof(unsigned) * InternalBuffer.size());

			std::vector<unsigned> sizebuffer;
			for(std::vector<std::vector<T> >::const_iterator iter = InternalBuffer.begin(); iter != InternalBuffer.end(); ++iter)
				sizebuffer.push_back(static_cast<unsigned>(iter->size()));

			cuMemAlloc(&SizesDevicePointer, sizeinbytes);
			cuMemcpyHtoD(SizesDevicePointer, &sizebuffer[0], sizeinbytes);
		}
	}

// Data transfer operations
public:
	void PassVariablesToDevice(const std::list<Traverser::ScopeContents>& variables, HandleType activatedscopehandle)
	{
		InternalBuffer.clear();

		for(std::list<Traverser::ScopeContents>::const_iterator iter = variables.begin(); iter != variables.end(); ++iter)
		{
			if(iter->Type == VM::EpochVariableType_Array && iter->ContainedType == DataType)
			{
				Traverser::Payload payload;
				FugueVMAccess::Interface.MarshalRead(activatedscopehandle, iter->Identifier, &payload);
				InternalBuffer.push_back(std::vector<T>(payload.ParameterCount));
				for(size_t i = 0; i < payload.ParameterCount; ++i)
					InternalBuffer.back()[i] = *(reinterpret_cast<T*>(payload.PointerValue) + i);
			}
		}

		if(DevicePointer)
		{
			cuMemFree(DevicePointer);
			DevicePointer = 0;
		}

		if(!InternalBuffer.empty())
		{
			size_t buffersize = 0;
			for(std::vector<std::vector<T> >::const_iterator iter = InternalBuffer.begin(); iter != InternalBuffer.end(); ++iter)
				buffersize += iter->size();

			unsigned buffersizeinbytes = static_cast<unsigned>(buffersize * sizeof(T));

			unsigned index = 0;
			std::vector<T> flatbuffer(buffersize);
			for(std::vector<std::vector<T> >::const_iterator iter = InternalBuffer.begin(); iter != InternalBuffer.end(); ++iter)
			{
				for(std::vector<T>::const_iterator inneriter = iter->begin(); inneriter != iter->end(); ++inneriter)
				{
					flatbuffer[index] = *inneriter;
					++index;
				}
			}
			
			cuMemAlloc(&DevicePointer, buffersizeinbytes);
			cuMemcpyHtoD(DevicePointer, &flatbuffer[0], buffersizeinbytes);
		}
	}

	void RetrieveVariablesFromDevice(const std::list<Traverser::ScopeContents>& variables, HandleType activatedscopehandle)
	{
		if(InternalBuffer.empty())
			return;

		size_t buffersize = 0;
		for(std::vector<std::vector<T> >::const_iterator iter = InternalBuffer.begin(); iter != InternalBuffer.end(); ++iter)
			buffersize += iter->size();

		unsigned buffersizeinbytes = static_cast<unsigned>(buffersize * sizeof(T));

		std::vector<T> flatbuffer(buffersize);
		cuMemcpyDtoH(&flatbuffer[0], DevicePointer, buffersizeinbytes);

		size_t index = 0;
		size_t internalindex = 0;

		for(std::list<Traverser::ScopeContents>::const_iterator iter = variables.begin(); iter != variables.end(); ++iter)
		{
			if(iter->Type == VM::EpochVariableType_Array && iter->ContainedType == DataType)
			{
				Traverser::Payload payload;
				payload.Type = VM::EpochVariableType_Array;
				payload.PointerValue = &(flatbuffer[index]);
				payload.ParameterCount = InternalBuffer[internalindex].size();
				payload.ParameterType = iter->ContainedType;
				FugueVMAccess::Interface.MarshalWrite(activatedscopehandle, iter->Identifier, &payload);
				index += InternalBuffer[internalindex].size();
				++internalindex;
			}
		}

		cuMemFree(DevicePointer);
		DevicePointer = 0;
	}

// Additional accessors
public:
	CUdeviceptr GetDevicePointer() const
	{ return DevicePointer; }

	size_t GetNumArrays() const
	{ return InternalBuffer.size(); }

	CUdeviceptr GetSizesBufferPointer() const
	{ return SizesDevicePointer; }

// Internal tracking
private:
	CUdeviceptr DevicePointer;
	CUdeviceptr SizesDevicePointer;
	std::vector<std::vector<T> > InternalBuffer;
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
	SynchronizableBuffer<Integer32, VM::EpochVariableType_Integer> SyncBufferForInts;

	SynchronizableArrayBuffer<Real, VM::EpochVariableType_Real> SyncBufferForRealArrays;
	SynchronizableArrayBuffer<Integer32, VM::EpochVariableType_Integer> SyncBufferForIntArrays;
};

