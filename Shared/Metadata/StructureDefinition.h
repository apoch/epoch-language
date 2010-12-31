//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing the contents of a structure data type
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/FunctionSignature.h"

#include <vector>
#include <map>


class StructureDefinition
{
// Construction
public:
	StructureDefinition()
		: Offset(0),
		  MarshaledSize(0)
	{ }

// Member configuration
public:
	void AddMember(StringHandle identifier, VM::EpochTypeID type, const StructureDefinition* structdefinition);

// Member accessors
public:
	size_t GetNumMembers() const;
	StringHandle GetMemberName(size_t index) const;
	VM::EpochTypeID GetMemberType(size_t index) const;
	size_t GetMemberOffset(size_t index) const;
	size_t FindMember(StringHandle identifier) const;

// Additional accessors
public:
	size_t GetSize() const
	{ return Offset; }

	size_t GetMarshaledSize() const
	{ return MarshaledSize; }

// Public tracking
public:
	FunctionSignatureSet FunctionSignatures;

// Internal helper
private:
	struct MemberRecord
	{
		StringHandle Identifier;
		VM::EpochTypeID Type;
		size_t Offset;

		MemberRecord(StringHandle identifier, VM::EpochTypeID type, size_t offset)
			: Identifier(identifier),
			  Type(type),
			  Offset(offset)
		{ }
	};

	typedef std::vector<MemberRecord> MemberList;

// Internal tracking
private:
	MemberList Members;
	size_t Offset;
	size_t MarshaledSize;
};


// Handy type shortcuts
typedef std::map<VM::EpochTypeID, StructureDefinition> StructureDefinitionMap;
typedef std::map<StringHandle, VM::EpochTypeID> StructureNameMap;
