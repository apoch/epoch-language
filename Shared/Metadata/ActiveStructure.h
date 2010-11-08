//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for managing an active structure in memory
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

// Data access
public:
	template<typename T>
	T ReadMember(size_t index) const
	{
		const UByte* rawptr = &Storage[0] + Definition.GetMemberOffset(index);
		const T* ptr = reinterpret_cast<const T*>(rawptr);
		return *ptr;
	}

	template<typename T>
	void WriteMember(size_t index, T value)
	{
		UByte* rawptr = &Storage[0] + Definition.GetMemberOffset(index);
		T* ptr = reinterpret_cast<T*>(rawptr);
		*ptr = value;
	}

// Original structure definition
public:
	const StructureDefinition& Definition;
	std::vector<UByte> Storage;
};

