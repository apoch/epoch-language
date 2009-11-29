//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for storing string values
//

#pragma once


#include "Virtual Machine/Core Entities/Variables/Variable.h"


namespace VM
{


	//
	// Wrapper for string-typed variables
	//
	class StringVariable : public Variable
	{
	// Handy type shortcut
	public:
		typedef HandleType BaseStorage;

	// Construction
	public:
		StringVariable(void* storage)
			: Variable(EpochVariableType_String, storage)
		{
		}

		StringVariable(void* storage, RValuePtr rvalue)
			: Variable(EpochVariableType_String, storage)
		{
			SetValue(rvalue->CastTo<StringRValue>().GetValue(), true);
		}

	// Variable interface
	public:
		const std::wstring& GetValue() const
		{
			HandleType id = *reinterpret_cast<HandleType*>(Storage);
			if(!id)
				throw InternalFailureException("Cannot retrieve value of unassigned string");
			return Pool.Get(id);
		}

		RValuePtr GetAsRValue() const
		{ return RValuePtr(new StringRValue(GetValue())); }

		const std::wstring& SetValue(const std::wstring& newvalue, bool ignorestorage)
		{
			if(ignorestorage)
			{
				HandleType id = Pool.Add(newvalue);
				*reinterpret_cast<HandleType*>(Storage) = id;
			}
			else
			{
				HandleType id = *reinterpret_cast<HandleType*>(Storage);
				if(id)
					Pool.Set(id, newvalue);
				else
				{
					id = Pool.Add(newvalue);
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
		{ return EpochVariableType_String; }

	// Pooling of strings
	public:
		static HandleType PoolStringLiteral(const std::wstring& value)
		{
			return Pool.Add(value);
		}

		static const std::wstring& GetByHandle(HandleType handle)
		{
			return Pool.Get(handle);
		}

		static void EmptyPool()
		{
			Pool.Clear();
		}

	// Internal helper class for pooling strings
	protected:

		//
		// Strings in Epoch are mutable and variable length; therefore, we cannot directly
		// pass the string's data block on the stack. In order to work around this, we pool
		// all string data in this pool class, and string variables are simply ID pointers
		// into the pool. The garbage collector takes care of freeing string data which is
		// no longer in use.
		//
		class PoolType
		{
		public:
			PoolType()
				: CurID(0)
			{ }

			~PoolType()
			{
				for(std::map<HandleType, std::wstring*>::const_iterator iter = ThePool.begin(); iter != ThePool.end(); ++iter)
					delete iter->second;
			}

			HandleType Add(const std::wstring& value)
			{
				++CurID;
				ThePool.insert(std::make_pair(CurID, new std::wstring(value)));
				return CurID;
			}
			void Set(HandleType id, const std::wstring& value)
			{
				if(ThePool.find(id) == ThePool.end())
					throw InternalFailureException("Cannot set mutable string entry - ID not allocated");

				delete ThePool[id];
				ThePool[id] = new std::wstring(value);
			}
			const std::wstring& Get(HandleType id) const
			{
				std::map<HandleType, std::wstring*>::const_iterator iter = ThePool.find(id);
				if(iter == ThePool.end())
					throw InternalFailureException("Invalid pooled string ID!");

				return *(iter->second);
			}

			void Clear()
			{
				for(std::map<HandleType, std::wstring*>::iterator iter = ThePool.begin(); iter != ThePool.end(); ++iter)
					delete iter->second;
				ThePool.clear();
				CurID = 0;
			}

		protected:
			HandleType CurID;
			std::map<HandleType, std::wstring*> ThePool;
		};

		static PoolType Pool;
	};


}


