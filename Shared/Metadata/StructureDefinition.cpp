//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing the contents of a structure data type
//

#include "pch.h"

#include "Metadata/StructureDefinition.h"
#include "Metadata/Variant.h"

#include "Virtual Machine/TypeInfo.h"


//
// Add a data member to the structure
//
void StructureDefinition::AddMember(StringHandle identifier, VM::EpochTypeID type, const StructureDefinition* structdefinition, const VariantDefinition* variantdefinition)
{
	if(variantdefinition && VM::GetTypeFamily(type) == VM::EpochTypeFamily_SumType)
	{
		Members.push_back(MemberRecord(identifier, type, Offset));
		Offset += variantdefinition->GetMaxSize();
		MarshaledSize += variantdefinition->GetMaxSize();
	}
	else
	{
		Members.push_back(MemberRecord(identifier, type, Offset));
		Offset += VM::GetStorageSize(type);

		if(VM::GetTypeFamily(type) == VM::EpochTypeFamily_Structure || VM::GetTypeFamily(type) == VM::EpochTypeFamily_TemplateInstance)
			MarshaledSize += structdefinition->GetMarshaledSize();
		else
			MarshaledSize += VM::GetMarshaledSize(type);
	}
}


//
// Retrieve the number of members in the structure
//
size_t StructureDefinition::GetNumMembers() const
{
	return Members.size();
}

//
// Retrieve the type of a given member
//
VM::EpochTypeID StructureDefinition::GetMemberType(size_t index) const
{
	return Members[index].Type;
}

//
// Retrieve the name of a given member
//
StringHandle StructureDefinition::GetMemberName(size_t index) const
{
	return Members[index].Identifier;
}

//
// Retrieve the byte offset of a given member
//
size_t StructureDefinition::GetMemberOffset(size_t index) const
{
	return Members[index].Offset;
}

//
// Locate the index of the member with the given name
//
size_t StructureDefinition::FindMember(StringHandle identifier) const
{
	for(size_t i = 0; i < Members.size(); ++i)
	{
		if(Members[i].Identifier == identifier)
			return i;
	}

	throw FatalException("Invalid structure member");
}

