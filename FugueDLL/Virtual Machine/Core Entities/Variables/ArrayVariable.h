//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for storing arrays of values
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"


namespace VM
{

	//
	// Wrapper for array containers
	//
	class ArrayVariable : public Variable
	{
	// Handy type shortcut
	public:
		typedef HandleType BaseStorage;

	// Friend access for sharing handles with rvalues
	public:
		friend class ArrayRValue;

	// Construction
	public:
		ArrayVariable(void* storage)
			: Variable(EpochVariableType_Array, storage)
		{
		}

	// Variable interface
	public:
		HandleType GetValue() const
		{
			HandleType* pstorage = reinterpret_cast<HandleType*>(Storage);
			return *pstorage;
		}

		void SetValue(HandleType newvalue)
		{
			HandleType* pstorage = reinterpret_cast<HandleType*>(Storage);
			*pstorage = newvalue;
		}

		RValuePtr GetAsRValue() const
		{
			return RValuePtr(new ArrayRValue(GetValue(), false));		// TODO - replace this with "true" if arrays suddenly get borked
		}

		size_t BindToStack(StackSpace& stack)
		{
			stack.Push(GetStorageSize());
			Storage = stack.GetCurrentTopOfStack();
			return GetStorageSize();
		}

		VM::EpochVariableTypeID GetElementType() const
		{
			return Pool.Get(GetValue()).Type;
		}

		size_t GetNumElements() const;


	// Shared storage size/type retrieval
	public:
		size_t GetStorageSize()
		{ return sizeof(BaseStorage); }

		static size_t GetBaseStorageSize()
		{ return sizeof(BaseStorage); }

		static EpochVariableTypeID GetStaticType()
		{ return EpochVariableType_Array; }


	// Handle management
	public:
		static BaseStorage AllocateNewHandle(VM::EpochVariableTypeID elementtype, size_t numentries);

		static void* GetArrayStorage(BaseStorage id)
		{
			return Pool.Get(id).Buffer;
		}

	// Internal helper class for pooling array data
	protected:

		class PoolType
		{
			friend class ArrayRValue;

		protected:
			struct PoolEntry
			{
				Byte* Buffer;
				size_t Size;
				VM::EpochVariableTypeID Type;
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

			HandleType Add(const Byte* existingbuffer, size_t size, VM::EpochVariableTypeID type)
			{
				++CurID;
				PoolEntry entry;
				entry.Buffer = new Byte[size];
				entry.Size = size;
				entry.Type = type;
				if(existingbuffer)
					memcpy(entry.Buffer, existingbuffer, size);
				ThePool.insert(std::make_pair(CurID, entry));
				return CurID;
			}
			void Set(HandleType id, const Byte* existingbuffer, size_t size)
			{
				if(ThePool.find(id) == ThePool.end())
					throw InternalFailureException("Cannot set mutable array entry - ID not allocated");

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
					throw InternalFailureException("Invalid pooled array ID!");

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
