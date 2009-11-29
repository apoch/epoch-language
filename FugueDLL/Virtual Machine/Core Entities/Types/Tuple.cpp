//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Class encapsulating a tuple type. This class primarily
// is used to ensure that members of a tuple are set up
// correctly in memory, and read from/written to easily.
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Types/Tuple.h"
#include "Virtual Machine/Core Entities/Variables/TupleVariable.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/VMExceptions.h"


using namespace VM;


const IDType TupleTrackerClass::InvalidID = 0;

IDType TupleTrackerClass::CurrentID = 0;
std::map<IDType, TupleTrackerClass*> TupleTrackerClass::OwnerMap;


//-------------------------------------------------------------------------------
// Tuple type class
//-------------------------------------------------------------------------------

//
// Compute offsets of each tuple member
//
void TupleType::ComputeOffsets(const ScopeDescription& scope)
{
	size_t offset = sizeof(size_t);		// Skip the type annotation
	for(std::vector<std::wstring>::const_iterator iter = MemberOrder.begin(); iter != MemberOrder.end(); ++iter)
	{
		MemberInfoMap[*iter].Offset = offset;
		offset += TypeInfo::GetStorageSize(MemberInfoMap[*iter].Type);
	}
}



//-------------------------------------------------------------------------------
// Tuple type tracker class
//-------------------------------------------------------------------------------

//
// Clean up the tuple tracker and free all used memory
//
TupleTrackerClass::~TupleTrackerClass()
{
	for(std::map<IDType, TupleType*>::iterator iter = TupleTypeMap.begin(); iter != TupleTypeMap.end(); ++iter)
		delete iter->second;
}

//
// Discard our references to any memory storage so as to not
// cause double-deletion when the destructor runs
//
void TupleTrackerClass::Detach()
{
	TupleTypeMap.clear();
}

//
// Register a new tuple type
//
IDType TupleTrackerClass::RegisterTupleType(const TupleType& type)
{
	IDType id = LookForMatchingTupleType(type);
	if(id != InvalidID)
		return id;

	TupleTypeMap[++CurrentID] = new TupleType(type);
	OwnerMap[CurrentID] = this;
	return CurrentID;
}

//
// Find out if the given tuple type has already been registered.
// If so, return its ID; otherwise, return an invalid ID.
//
// Note that tuples only match if their members are all of the
// same types, in the same order, and using the same identifiers.
//
IDType TupleTrackerClass::LookForMatchingTupleType(const TupleType& type)
{
	for(std::map<IDType, TupleTrackerClass*>::const_iterator iter = OwnerMap.begin(); iter != OwnerMap.end(); ++iter)
	{
		const TupleType& tupletype = iter->second->GetTupleType(iter->first);
		const std::vector<std::wstring>& memberorder = tupletype.GetMemberOrder();
		
		if(memberorder == type.GetMemberOrder())
		{
			bool match = true;
			for(std::vector<std::wstring>::const_iterator memberiter = memberorder.begin(); memberiter != memberorder.end(); ++memberiter)
			{
				if(tupletype.GetMemberType(*memberiter) != type.GetMemberType(*memberiter))
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
// Find out if the given scope matches a registered tuple type.
// This assumes that the scope specifies the return values of a function.
// If a match is found, the ID of the type is returned; otherwise, returns an invalid ID.
//
IDType TupleTrackerClass::LookForMatchingTupleType(const ScopeDescription& scope)
{
	for(std::map<IDType, TupleTrackerClass*>::const_iterator iter = OwnerMap.begin(); iter != OwnerMap.end(); ++iter)
	{
		const TupleType& tupletype = iter->second->GetTupleType(iter->first);
		const std::vector<std::wstring>& memberorder = tupletype.GetMemberOrder();
		
		if(memberorder == scope.GetMemberOrder())
		{
			bool match = true;
			for(std::vector<std::wstring>::const_iterator memberiter = memberorder.begin(); memberiter != memberorder.end(); ++memberiter)
			{
				if(tupletype.GetMemberType(*memberiter) != scope.GetVariableType(*memberiter))
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
// Find out if the given set of return value types matches a registered tuple type.
//
IDType TupleTrackerClass::LookForMatchingTupleType(const std::vector<EpochVariableTypeID>& typelist)
{
	for(std::map<IDType, TupleTrackerClass*>::const_iterator iter = OwnerMap.begin(); iter != OwnerMap.end(); ++iter)
	{
		const TupleType& tupletype = iter->second->GetTupleType(iter->first);
		const std::vector<std::wstring>& memberorder = tupletype.GetMemberOrder();

		if(memberorder.size() != typelist.size())
			continue;
		
		bool match = true;
		for(unsigned i = 0; i < memberorder.size(); ++i)
		{
			if(tupletype.GetMemberType(memberorder[i]) != typelist[i])
			{
				match = false;
				break;
			}
		}

		if(match)
			return iter->first;
	}

	return InvalidID;
}

//
// Retrieve the requested tuple type structure
//
const TupleType& TupleTrackerClass::GetTupleType(IDType id) const
{
	if(id == InvalidID)
		throw InternalFailureException("The requested tuple type ID is invalid!");
	
	std::map<IDType, TupleTrackerClass*>::const_iterator iter = OwnerMap.find(id);
	if(iter == OwnerMap.end())
		throw InternalFailureException("The requested tuple type ID is invalid!");

	if(iter->second == this)
		return *TupleTypeMap.find(id)->second;
	
	return iter->second->GetTupleType(id);
}

//
// Locate the tracker class that owns the given tuple type
//
TupleTrackerClass* TupleTrackerClass::GetOwnerOfTupleType(IDType id)
{
	std::map<IDType, TupleTrackerClass*>::const_iterator iter = OwnerMap.find(id);
	if(iter == OwnerMap.end())
		throw InternalFailureException("The requested tuple type ID is invalid!");

	return iter->second;
}

//
// Reset the shared table for a new program to be loaded
//
void TupleTrackerClass::ResetSharedData()
{
	CurrentID = 0;
	OwnerMap.clear();
}


//-------------------------------------------------------------------------------
// Tuple variable class
//-------------------------------------------------------------------------------


//
// Locate information about tuple types
//
size_t TupleVariable::GetStorageSize() const
{
	TupleTrackerClass* tracker = TupleTrackerClass::GetOwnerOfTupleType(GetValue());
	return tracker->GetTupleType(GetValue()).GetTotalSize();
}

//
// Bind a tuple variable to a particular bit of stack memory
//
size_t TupleVariable::BindToStack(StackSpace& stack, const TupleType& typeinfo)
{
	stack.Push(typeinfo.GetTotalSize());
	Storage = stack.GetCurrentTopOfStack();
	return typeinfo.GetTotalSize();
}

//
// Read a member from a tuple
//
RValuePtr TupleVariable::ReadMember(const std::wstring& member) const
{
	TupleTrackerClass* tracker = TupleTrackerClass::GetOwnerOfTupleType(GetValue());
	void* storage = reinterpret_cast<Byte*>(Storage) + tracker->GetTupleType(GetValue()).GetMemberOffset(member);

	switch(tracker->GetTupleType(GetValue()).GetMemberType(member))
	{
	case EpochVariableType_Integer:
		{
			IntegerVariable var(storage);
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
	default:
		throw NotImplementedException("Cannot read tuple member of this type");
	}
}

//
// Assign a tuple member to a particlar value
//
void TupleVariable::WriteMember(const std::wstring& member, RValuePtr value, bool ignorestorage)
{
	TupleTrackerClass* tracker = TupleTrackerClass::GetOwnerOfTupleType(GetValue());
	void* storage = reinterpret_cast<Byte*>(Storage) + tracker->GetTupleType(GetValue()).GetMemberOffset(member);

	EpochVariableTypeID membertype = tracker->GetTupleType(GetValue()).GetMemberType(member);

	if(value->GetType() != membertype)
		throw ExecutionException("Type mismatch - cannot assign tuple member value");

	switch(membertype)
	{
	case EpochVariableType_Integer:
		{
			IntegerVariable var(storage);
			var.SetValue(value->CastTo<IntegerRValue>().GetValue());
		}
		break;
	case EpochVariableType_Real:
		{
			RealVariable var(storage);
			var.SetValue(value->CastTo<RealRValue>().GetValue());
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
			BooleanVariable var(storage);
			var.SetValue(value->CastTo<BooleanRValue>().GetValue());
		}
		break;
	default:
		throw NotImplementedException("Cannot assign to tuple member of this type");
	}
}

