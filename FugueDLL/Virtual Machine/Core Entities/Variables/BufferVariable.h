//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for storing byte buffers on the freestore
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"


namespace VM
{

	//
	// Wrapper for generic byte buffer objects
	//
	class BufferVariable : public Variable
	{
	// Handy type shortcut
	public:
		typedef HandleType BaseStorage;

	// Friend access for sharing handles with rvalues
	public:
		friend class BufferRValue;

	// Construction
	public:
		BufferVariable(void* storage)
			: Variable(EpochVariableType_Buffer, storage)
		{
		}

	// Variable interface
	public:
		Byte* GetValue() const
		{
			HandleType id = *reinterpret_cast<HandleType*>(Storage);
			if(!id)
				throw InternalFailureException("Cannot retrieve value of unassigned buffer");
			return Pool.Get(id).Buffer;
		}

		size_t GetSize() const
		{
			HandleType id = *reinterpret_cast<HandleType*>(Storage);
			if(!id)
				throw InternalFailureException("Cannot retrieve size of unassigned buffer");
			return Pool.Get(id).Size;
		}

		RValuePtr GetAsRValue() const
		{ return RValuePtr(new BufferRValue(GetHandleValue())); }

		Byte* SetValue(const Byte* existingbuffer, size_t newsize, bool ignorestorage)
		{
			if(ignorestorage)
			{
				HandleType id = Pool.Add(existingbuffer, newsize);
				*reinterpret_cast<HandleType*>(Storage) = id;
			}
			else
			{
				HandleType id = *reinterpret_cast<HandleType*>(Storage);
				if(id)
					Pool.Set(id, existingbuffer, newsize);
				else
				{
					id = Pool.Add(existingbuffer, newsize);
					*reinterpret_cast<HandleType*>(Storage) = id;
				}
			}
			return GetValue();
		}

		size_t BindToStack(StackSpace& stack)
		{
			stack.Push(GetStorageSize());
			Storage = stack.GetCurrentTopOfStack();
			return GetStorageSize();
		}

	// Direct handle manipulation - use sparingly!
	public:
		BaseStorage GetHandleValue() const
		{ return *reinterpret_cast<HandleType*>(Storage); }

		void SetHandleValue(BaseStorage newval)
		{ *reinterpret_cast<HandleType*>(Storage) = newval; }

	// Shared storage size/type retrieval
	public:
		static size_t GetStorageSize()
		{ return sizeof(size_t); }

		static size_t GetBaseStorageSize()
		{ return GetStorageSize(); }

		static EpochVariableTypeID GetStaticType()
		{ return EpochVariableType_Buffer; }

	// Internal helper class for pooling buffer data
	protected:

		class PoolType
		{
		protected:
			struct PoolEntry
			{
				Byte* Buffer;
				size_t Size;
			};

		public:
			PoolType()
				: CurID(0)
			{ }

			~PoolType()
			{
				for(std::map<HandleType, PoolEntry>::const_iterator iter = ThePool.begin(); iter != ThePool.end(); ++iter)
					delete [] iter->second.Buffer;
			}

			HandleType Add(const Byte* existingbuffer, size_t size)
			{
				++CurID;
				PoolEntry entry;
				entry.Buffer = new Byte[size];
				entry.Size = size;
				if(existingbuffer)
					memcpy(entry.Buffer, existingbuffer, size);
				ThePool.insert(std::make_pair(CurID, entry));
				return CurID;
			}
			void Set(HandleType id, const Byte* existingbuffer, size_t size)
			{
				if(ThePool.find(id) == ThePool.end())
					throw InternalFailureException("Cannot set mutable buffer entry - ID not allocated");

				delete [] ThePool[id].Buffer;
				ThePool[id].Buffer = new Byte[size];
				ThePool[id].Size = size;
				if(existingbuffer)
					memcpy(ThePool[id].Buffer, existingbuffer, size);
			}
			const PoolEntry& Get(HandleType id) const
			{
				std::map<HandleType, PoolEntry>::const_iterator iter = ThePool.find(id);
				if(iter == ThePool.end())
					throw InternalFailureException("Invalid pooled buffer ID!");

				return iter->second;
			}

			void Clear()
			{
				for(std::map<HandleType, PoolEntry>::iterator iter = ThePool.begin(); iter != ThePool.end(); ++iter)
					delete [] iter->second.Buffer;
				ThePool.clear();
				CurID = 0;
			}

		protected:
			HandleType CurID;
			std::map<HandleType, PoolEntry> ThePool;
		};

		static PoolType Pool;
	};

}
