//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for storing structures
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"


namespace VM
{

	//
	// Wrapper for structures
	//
	class StructureVariable : public Variable
	{
	// Handy type shortcut
	public:
		typedef IDType BaseStorage;

	// Construction
	public:
		StructureVariable(void* storage)
			: Variable(EpochVariableType_Structure, storage)
		{
		}

	// Variable interface
	public:
		IDType GetValue() const
		{
			IDType id = *reinterpret_cast<IDType*>(Storage);
			if(!id)
				throw InternalFailureException("Structure variable is missing type information");
			return id;
		}

		IDType SetValue(IDType newvalue)
		{
			*reinterpret_cast<IDType*>(Storage) = newvalue;
			return GetValue();
		}

		size_t BindToStack(StackSpace& stack, const StructureType& typeinfo);

	// Structure interface
	public:
		RValuePtr ReadMember(const std::wstring& member) const;

		void WriteMember(const std::wstring& member, RValuePtr value, bool ignorestorage);
		
		template<class TypeData>
		void WriteMember(const std::wstring& member, void* value, bool ignorestorage)
		{
			WriteMember(member, RValuePtr(new TypeData::RValueType(*reinterpret_cast<TypeData::VariableType::BaseStorage*>(value))), ignorestorage);
		}

	// Shared storage size/type retrieval
	public:
	 	size_t GetStorageSize() const;

		static size_t GetBaseStorageSize()
		{ return sizeof(BaseStorage); }

		static EpochVariableTypeID GetStaticType()
		{ return EpochVariableType_Structure; }
	};

}

