//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Class encapsulating a structure type. This class primarily
// is used to ensure that members of a structure are set up
// correctly in memory, and read from/written to easily.
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Types/Structure.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/VMExceptions.h"


using namespace VM;


const IDType StructureTrackerClass::InvalidID = 0;

IDType StructureTrackerClass::CurrentID = 0;
std::map<IDType, StructureTrackerClass*> StructureTrackerClass::OwnerMap;


//-------------------------------------------------------------------------------
// Structure type class
//-------------------------------------------------------------------------------

//
// Compute offsets of each structure member
//
void StructureType::ComputeOffsets(const ScopeDescription& scope)
{
	size_t offset = sizeof(size_t);		// Skip the type annotation
	StorageSize = 0;
	for(std::vector<std::wstring>::const_iterator iter = MemberOrder.begin(); iter != MemberOrder.end(); ++iter)
	{
		size_t storagesize;
		switch(MemberInfoMap[*iter].Type)
		{
		case EpochVariableType_Structure:
			storagesize = scope.GetStructureType(MemberInfoMap[*iter].TypeHint).GetTotalSize();
			break;
		case EpochVariableType_Tuple:
			storagesize = scope.GetTupleType(MemberInfoMap[*iter].TypeHint).GetTotalSize();
			break;
		default:
			storagesize = TypeInfo::GetStorageSize(MemberInfoMap[*iter].Type);
			break;
		}

		// TODO - more intelligent packing/padding code
		/*
		// PLATFORM DEPENDENT CODE - PAD TO 32 BITS
		if(storagesize > 4)			// Tail-padding for blobs larger than 32 bits
		{
			MemberInfoMap[*iter].Offset = offset;
			offset += storagesize;
			StorageSize += storagesize;
			while(offset % 4 != 0)
			{
				++offset;
				++StorageSize;
			}
		}
		else if(storagesize < 4)	// Head-padding for blobs less than 32 bits
		{
			size_t pad = 4 - storagesize;
			offset += pad;
			MemberInfoMap[*iter].Offset = offset;
			offset += storagesize;
			StorageSize += 4;
		}
		else
		{
			MemberInfoMap[*iter].Offset = offset;
			offset += storagesize;
			StorageSize += 4;
		}
		// END PLATFORM DEPENDENCY
		*/

		MemberInfoMap[*iter].Offset = offset;
		offset += storagesize;
		StorageSize += storagesize;
	}
}


//-------------------------------------------------------------------------------
// Structure type tracker class
//-------------------------------------------------------------------------------

//
// Clean up the structure tracker and free all used memory
//
StructureTrackerClass::~StructureTrackerClass()
{
	for(std::map<IDType, StructureType*>::iterator iter = StructureTypeMap.begin(); iter != StructureTypeMap.end(); ++iter)
		delete iter->second;
}

//
// Discard our references to any memory storage so as to not
// cause double-deletion when the destructor runs
//
void StructureTrackerClass::Detach()
{
	StructureTypeMap.clear();
}

//
// Register a new structure type
//
IDType StructureTrackerClass::RegisterStructureType(const StructureType& type)
{
	IDType id = LookForMatchingStructureType(type);
	if(id != InvalidID)
		return id;

	StructureTypeMap[++CurrentID] = new StructureType(type);
	OwnerMap[CurrentID] = this;
	return CurrentID;
}

//
// Find out if the given structure type has already been registered.
// If so, return its ID; otherwise, return an invalid ID.
//
// Note that structures only match if their members are all of the
// same types, in the same order, and using the same identifiers.
//
IDType StructureTrackerClass::LookForMatchingStructureType(const StructureType& type)
{
	for(std::map<IDType, StructureTrackerClass*>::const_iterator iter = OwnerMap.begin(); iter != OwnerMap.end(); ++iter)
	{
		const StructureType& structuretype = iter->second->GetStructureType(iter->first);
		const std::vector<std::wstring>& memberorder = structuretype.GetMemberOrder();
		
		if(memberorder == type.GetMemberOrder())
		{
			bool match = true;
			for(std::vector<std::wstring>::const_iterator memberiter = memberorder.begin(); memberiter != memberorder.end(); ++memberiter)
			{
				if(structuretype.GetMemberType(*memberiter) != type.GetMemberType(*memberiter))
				{
					match = false;
					break;
				}
			}

			if(match)
				return iter->first;
		}
	}

	return InvalidID;
}

//
// Find out if the given scope matches a registered structure type.
// This assumes that the scope specifies the return values of a function.
// If a match is found, the ID of the type is returned; otherwise, returns an invalid ID.
//
IDType StructureTrackerClass::LookForMatchingStructureType(const ScopeDescription& scope)
{
	for(std::map<IDType, StructureTrackerClass*>::const_iterator iter = OwnerMap.begin(); iter != OwnerMap.end(); ++iter)
	{
		const StructureType& structuretype = iter->second->GetStructureType(iter->first);
		const std::vector<std::wstring>& memberorder = structuretype.GetMemberOrder();
		
		if(memberorder == scope.GetMemberOrder())
		{
			bool match = true;
			for(std::vector<std::wstring>::const_iterator memberiter = memberorder.begin(); memberiter != memberorder.end(); ++memberiter)
			{
				if(structuretype.GetMemberType(*memberiter) != scope.GetVariableType(*memberiter))
				{
					match = false;
					break;
				}
			}

			if(match)
				return iter->first;
		}
	}

	return InvalidID;
}

//
// Retrieve the requested structure type definition
//
const StructureType& StructureTrackerClass::GetStructureType(IDType id) const
{
	if(id == InvalidID)
		throw InternalFailureException("The requested structure type ID is invalid!");
	
	std::map<IDType, StructureTrackerClass*>::const_iterator iter = OwnerMap.find(id);
	if(iter == OwnerMap.end())
		throw InternalFailureException("The requested structure type ID is invalid!");

	if(iter->second == this)
		return *StructureTypeMap.find(id)->second;
	
	return iter->second->GetStructureType(id);
}

//
// Locate the tracker class that owns the given structure type
//
StructureTrackerClass* StructureTrackerClass::GetOwnerOfStructureType(IDType id)
{
	std::map<IDType, StructureTrackerClass*>::const_iterator iter = OwnerMap.find(id);
	if(iter == OwnerMap.end())
		throw InternalFailureException("The requested structure type ID is invalid!");

	return iter->second;
}

//
// Reset the shared table for a new program to be loaded
//
void StructureTrackerClass::ResetSharedData()
{
	CurrentID = 0;
	OwnerMap.clear();
}


//-------------------------------------------------------------------------------
// Structure variable class
//-------------------------------------------------------------------------------


//
// Locate information about structure types
//
size_t StructureVariable::GetStorageSize() const
{
	StructureTrackerClass* tracker = StructureTrackerClass::GetOwnerOfStructureType(GetValue());
	return tracker->GetStructureType(GetValue()).GetTotalSize();
}

//
// Bind a structure variable to a particular bit of stack memory
//
size_t StructureVariable::BindToStack(StackSpace& stack, const StructureType& typeinfo)
{
	stack.Push(typeinfo.GetTotalSize());
	Storage = stack.GetCurrentTopOfStack();
	return typeinfo.GetTotalSize();
}

//
// Read a member from a structure
//
RValuePtr StructureVariable::ReadMember(const std::wstring& member) const
{
	StructureTrackerClass* tracker = StructureTrackerClass::GetOwnerOfStructureType(GetValue());
	void* storage = reinterpret_cast<Byte*>(Storage) + tracker->GetStructureType(GetValue()).GetMemberOffset(member);

	switch(tracker->GetStructureType(GetValue()).GetMemberType(member))
	{
	case EpochVariableType_Integer:
		{
			IntegerVariable var(storage);
			return var.GetAsRValue();
		}
	case EpochVariableType_Integer16:
		{
			Integer16Variable var(storage);
			return var.GetAsRValue();
		}
	case EpochVariableType_Real:
		{
			RealVariable var(storage);
			return var.GetAsRValue();
		}
	case EpochVariableType_String:
		{
			StringVariable var(storage);
			return var.GetAsRValue();
		}
	case EpochVariableType_Boolean:
		{
			BooleanVariable var(storage);
			return var.GetAsRValue();
		}
	case EpochVariableType_Structure:
		{
			StructureVariable var(storage);
			IDType id = var.GetValue();
			const StructureType& structtype = StructureTrackerClass::GetOwnerOfStructureType(id)->GetStructureType(id);
			StructureRValue* rval = new StructureRValue(structtype, id);
			
			const std::vector<std::wstring>& members = structtype.GetMemberOrder();
			for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				rval->AddMember(*iter, RValuePtr(var.ReadMember(*iter)->Clone()));

			return RValuePtr(rval);
		}
	case EpochVariableType_Function:
		{
			FunctionBinding var(storage);
			return var.GetAsRValue();
		}
	default:
		throw NotImplementedException("Cannot read structure member of this type");
	}
}

//
// Assign a structure member to a particlar value
//
void StructureVariable::WriteMember(const std::wstring& member, RValuePtr value, bool ignorestorage)
{
	StructureTrackerClass* tracker = StructureTrackerClass::GetOwnerOfStructureType(GetValue());
	void* storage = reinterpret_cast<Byte*>(Storage) + tracker->GetStructureType(GetValue()).GetMemberOffset(member);

	EpochVariableTypeID membertype = tracker->GetStructureType(GetValue()).GetMemberType(member);

	if(value->GetType() != membertype)
		throw ExecutionException("Type mismatch - cannot assign structure member value");

	switch(membertype)
	{
	case EpochVariableType_Integer:
		{
			IntegerVariable var(storage, value);
		}
		break;
	case EpochVariableType_Integer16:
		{
			Integer16Variable var(storage, value);
		}
		break;
	case EpochVariableType_Real:
		{
			RealVariable var(storage, value);
		}
		break;
	case EpochVariableType_String:
		{
			StringVariable var(storage);
			var.SetValue(value->CastTo<StringRValue>().GetValue(), ignorestorage);
		}
		break;
	case EpochVariableType_Boolean:
		{
			BooleanVariable var(storage, value);
		}
		break;
	case EpochVariableType_Structure:
		{
			const StructureRValue& structrval = value->CastTo<StructureRValue>();
			IDType hint = structrval.GetStructureTypeID();

			StructureVariable var(storage);
			var.SetValue(hint);

			const std::vector<std::wstring>& members = StructureTrackerClass::GetOwnerOfStructureType(hint)->GetStructureType(hint).GetMemberOrder();
			for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				var.WriteMember(*iter, structrval.GetValue(*iter), ignorestorage);
		}
		break;
	case EpochVariableType_Function:
		{
			FunctionBinding var(storage, value);
		}
		break;
	default:
		throw NotImplementedException("Cannot assign to structure member of this type");
	}
}

