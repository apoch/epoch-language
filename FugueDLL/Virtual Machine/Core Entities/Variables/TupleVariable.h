//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for storing tuples
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"


namespace VM
{

	//
	// Wrapper for tuples
	//
	class TupleVariable : public Variable
	{
	// Handy type shortcut
	public:
		typedef IDType BaseStorage;

	// Construction
	public:
		TupleVariable(void* storage)
			: Variable(EpochVariableType_Tuple, storage)
		{
		}

	// Variable interface
	public:
		IDType GetValue() const
		{
			IDType id = *reinterpret_cast<IDType*>(Storage);
			if(!id)
				throw InternalFailureException("Tuple variable is missing type information");
			return id;
		}

		IDType SetValue(IDType newvalue)
		{
			*reinterpret_cast<IDType*>(Storage) = newvalue;
			return GetValue();
		}

		size_t BindToStack(StackSpace& stack, const TupleType& typeinfo);

	// Tuple interface
	public:
		RValuePtr ReadMember(const std::wstring& member) const;
		void WriteMember(const std::wstring& member, RValuePtr value, bool ignorestorage);

	// Shared storage size/type retrieval
	public:
	 	size_t GetStorageSize() const;

		static size_t GetBaseStorageSize()
		{ return sizeof(BaseStorage); }

		static EpochVariableTypeID GetStaticType()
		{ return EpochVariableType_Tuple; }
	};

}