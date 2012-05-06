//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for managing an active structure in memory
//
// Note that this class does not do any type checking or direct validation of
// other invariants; it is merely a safe wrapper for the VM implementation to
// access the raw memory storage used by a structure instance (aka object).
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"

#include "Metadata/StructureDefinition.h"

#include <vector>


class ActiveStructure
{
// Construction
public:
	explicit ActiveStructure(const StructureDefinition& definition)
		: Definition(definition),
		  Storage(definition.GetSize(), 0)
	{ }

// Assignment prohibited
private:
	ActiveStructure& operator = (const ActiveStructure& rhs);

// Data access
public:
	template<typename T>
	T ReadMember(size_t index) const
	{
		const UByte* rawptr = &Storage[0] + Definition.GetMemberOffset(index);

		if(VM::GetTypeFamily(Definition.GetMemberType(index)) == VM::EpochTypeFamily_SumType)
			rawptr += sizeof(VM::EpochTypeID);

		const T* ptr = reinterpret_cast<const T*>(rawptr);
		return *ptr;
	}

	template<typename T>
	void WriteMember(size_t index, T value)
	{
		UByte* rawptr = &Storage[0] + Definition.GetMemberOffset(index);

		if(VM::GetTypeFamily(Definition.GetMemberType(index)) == VM::EpochTypeFamily_SumType)
			rawptr += sizeof(VM::EpochTypeID);

		T* ptr = reinterpret_cast<T*>(rawptr);
		*ptr = value;
	}

	VM::EpochTypeID ReadSumTypeMemberType(size_t index) const
	{
		const UByte* rawptr = &Storage[0] + Definition.GetMemberOffset(index);
		const VM::EpochTypeID* ptr = reinterpret_cast<const VM::EpochTypeID*>(rawptr);
		return *ptr;
	}

	void WriteSumTypeMemberType(size_t index, VM::EpochTypeID type)
	{
		UByte* rawptr = &Storage[0] + Definition.GetMemberOffset(index);
		VM::EpochTypeID* typeptr = reinterpret_cast<VM::EpochTypeID*>(rawptr);
		*typeptr = type;
	}

// Original structure definition
public:
	const StructureDefinition& Definition;
	std::vector<UByte> Storage;
};

