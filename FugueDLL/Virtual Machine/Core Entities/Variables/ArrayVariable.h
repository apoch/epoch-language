//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for storing arrays of values
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Types Management/TypeInfo.h"


namespace VM
{

	//
	// Wrapper for array containers
	//
	class ArrayVariable : public Variable
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
		friend class ArrayRValue;

	// Construction
	public:
		ArrayVariable(void* storage)
			: Variable(EpochVariableType_Array, storage)
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
			ArrayRValue* p = new ArrayRValue(GetValue().ElementType);
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
						BooleanVariable var(offset);
						p->AddElement(var.GetAsRValue().release());
						offset = reinterpret_cast<Byte*>(offset) + var.GetStorageSize();
						break;
					}
				case EpochVariableType_String:
					{
						StringVariable var(offset);
						p->AddElement(var.GetAsRValue().release());
						offset = reinterpret_cast<Byte*>(offset) + var.GetStorageSize();
						break;
					}
				default:
					throw NotImplementedException("Arrays of this element type are not supported");
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

		size_t BindToStack(StackSpace& stack, size_t arraysize, EpochVariableTypeID elementtype)
		{
			Storage = stack.GetCurrentTopOfStack();
			return GetBaseStorageSize() + arraysize * TypeInfo::GetStorageSize(elementtype);
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
		{ return EpochVariableType_Array; }
	};

}
