//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for storing lists of values
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Types Management/TypeInfo.h"


namespace VM
{

	//
	// Wrapper for list containers
	//
	class ListVariable : public Variable
	{
	// Handy type shortcut
	public:
		struct StorageRecT
		{
			union
			{
				EpochVariableTypeID ElementType;
				unsigned Ignored;		// Make sure the enumeration takes a full int worth of space
			};
			size_t NumElements;
		};

		typedef StorageRecT BaseStorage;

	// Friend access for sharing handles with rvalues
	public:
		friend class ListRValue;

	// Construction
	public:
		ListVariable(void* storage)
			: Variable(EpochVariableType_List, storage)
		{
		}

	// Variable interface
	public:
		StorageRecT GetValue() const
		{
			StorageRecT* pstorage = reinterpret_cast<StorageRecT*>(Storage);
			return StorageRecT(*pstorage);
		}

		RValuePtr GetAsRValue() const
		{
			ListRValue* p = new ListRValue(GetValue().ElementType);
			RValuePtr ret(p);

			void* offset = reinterpret_cast<Byte*>(Storage) + sizeof(StorageRecT);
			
			StorageRecT* pstorage = reinterpret_cast<StorageRecT*>(Storage);
			for(unsigned i = 0; i < pstorage->NumElements; ++i)
			{
				switch(pstorage->ElementType)
				{
				case EpochVariableType_Integer:
					{
						IntegerVariable var(offset);
						p->AddElement(var.GetAsRValue().release());
						offset = reinterpret_cast<Byte*>(offset) + var.GetStorageSize();
						break;
					}
				case EpochVariableType_Integer16:
					{
						Integer16Variable var(offset);
						p->AddElement(var.GetAsRValue().release());
						offset = reinterpret_cast<Byte*>(offset) + var.GetStorageSize();
						break;
					}
				case EpochVariableType_Real:
					{
						RealVariable var(offset);
						p->AddElement(var.GetAsRValue().release());
						offset = reinterpret_cast<Byte*>(offset) + var.GetStorageSize();
						break;
					}
				case EpochVariableType_Boolean:
					{
						IntegerVariable var(offset);
						p->AddElement(var.GetAsRValue().release());
						offset = reinterpret_cast<Byte*>(offset) + var.GetStorageSize();
						break;
					}
				case EpochVariableType_String:
					{
						IntegerVariable var(offset);
						p->AddElement(var.GetAsRValue().release());
						offset = reinterpret_cast<Byte*>(offset) + var.GetStorageSize();
						break;
					}
				default:
					throw NotImplementedException("Lists of this element type are not supported");
				}
			}

			return ret;
		}

		void SetInfo(EpochVariableTypeID elementtype, size_t size)
		{
			StorageRecT* pstorage = reinterpret_cast<StorageRecT*>(Storage);
			pstorage->ElementType = elementtype;
			pstorage->NumElements = size;
		}

		size_t BindToStack(StackSpace& stack, size_t listsize, EpochVariableTypeID elementtype)
		{
			Storage = stack.GetCurrentTopOfStack();
			return GetBaseStorageSize() + listsize * TypeInfo::GetStorageSize(elementtype);
		}

	// Shared storage size/type retrieval
	public:
		size_t GetStorageSize()
		{
			return sizeof(StorageRecT) + GetValue().NumElements * TypeInfo::GetStorageSize(GetValue().ElementType);
		}

		static size_t GetBaseStorageSize()
		{ return sizeof(StorageRecT); }

		static EpochVariableTypeID GetStaticType()
		{ return EpochVariableType_List; }
	};

}
