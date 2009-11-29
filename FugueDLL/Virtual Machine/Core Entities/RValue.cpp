//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper classes and type definitions for working with r-values.
//

#include "pch.h"

#include "Virtual Machine/Core Entities/RValue.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/VMExceptions.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"


using namespace VM;



//-------------------------------------------------------------------------------
// Tuples
//-------------------------------------------------------------------------------

//
// Construct and initialize the tuple of values
//
TupleRValue::TupleRValue(const TupleType& tupletype, IDType tupletypeid)
	: RValue(EpochVariableType_Tuple),
	  TupleTypeID(tupletypeid)
{
}

//
// Construct and initialize the tuple of values
//
TupleRValue::TupleRValue(const TupleType& tupletype, const ActivatedScope& scope, IDType tupletypeid)
	: RValue(EpochVariableType_Tuple),
	  TupleTypeID(tupletypeid)
{
	const std::vector<std::wstring>& members = tupletype.GetMemberOrder();

	for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
		MemberData[*iter] = scope.GetVariableValue(*iter)->Clone();
}

//
// Destruct and clean up the tuple of values
//
TupleRValue::~TupleRValue()
{
	Clean();
}

//
// Add a member to the tuple
//
// Call this function with EXTREME care; don't use it on existing variables,
// but rather only on variables that are being set up/bound to storage. Any
// other usage will actually change the underlying type of the tuple, with
// unpredictable but likely nasty results.
//
void TupleRValue::AddMember(const std::wstring& membername, RValuePtr rvalptr)
{
	if(MemberData.find(membername) != MemberData.end())
		throw ExecutionException("Duplicate member name - tuple member names must all be different");

	MemberData[membername] = rvalptr.release();
}

//
// Retrieve the value of one of the tuple members
//
RValuePtr TupleRValue::GetValue(const std::wstring& membername) const
{
	std::map<std::wstring, RValue*>::const_iterator iter = MemberData.find(membername);
	if(iter == MemberData.end())
		throw ExecutionException("Specified identifier is not a member of this tuple type");

	return RValuePtr(iter->second->Clone());
}

//
// Compare two tuples to see if they are equal
//
bool TupleRValue::VirtualComparator(const RValue& rhs) const
{
	if(rhs.GetType() != EpochVariableType_Tuple)
		return false;

	const TupleRValue& other = rhs.CastTo<TupleRValue>();
	if(other.TupleTypeID != TupleTypeID)
		return false;

	if(other.MemberData.size() != MemberData.size())
		return false;

	for(std::map<std::wstring, RValue*>::const_iterator iter = MemberData.begin(); iter != MemberData.end(); ++iter)
	{
		std::map<std::wstring, RValue*>::const_iterator otheriter = other.MemberData.find(iter->first);
		if(otheriter == other.MemberData.end())
			return false;

		if(iter->second->GetType() != otheriter->second->GetType())
			return false;

		switch(iter->second->GetType())
		{
		case EpochVariableType_Integer:
			if(iter->second->CastTo<IntegerRValue>().GetValue() != otheriter->second->CastTo<IntegerRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Integer16:
			if(iter->second->CastTo<Integer16RValue>().GetValue() != otheriter->second->CastTo<Integer16RValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Real:
			if(iter->second->CastTo<RealRValue>().GetValue() != otheriter->second->CastTo<RealRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Boolean:
			if(iter->second->CastTo<BooleanRValue>().GetValue() != otheriter->second->CastTo<BooleanRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_String:
			if(iter->second->CastTo<StringRValue>().GetValue() != otheriter->second->CastTo<StringRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Tuple:
			if(!iter->second->CastTo<TupleRValue>().VirtualComparator(*otheriter->second))
				return false;
			break;
		case EpochVariableType_Structure:
			if(!iter->second->CastTo<TupleRValue>().VirtualComparator(*otheriter->second))
				return false;
			break;

		default:
			throw NotImplementedException("Cannot compare tuple members of this type");
		}
	}

	return true;
}

//
// Clean up all storage used by the r-value wrapper
//
void TupleRValue::Clean()
{
	for(std::map<std::wstring, RValue*>::iterator iter = MemberData.begin(); iter != MemberData.end(); ++iter)
		delete iter->second;

	MemberData.clear();
}

//
// Perform a deep copy from another tuple r-value
//
void TupleRValue::CopyFrom(const TupleRValue& rhs)
{
	TupleTypeID = rhs.TupleTypeID;

	for(std::map<std::wstring, RValue*>::const_iterator iter = rhs.MemberData.begin(); iter != rhs.MemberData.end(); ++iter)
		MemberData.insert(std::make_pair(iter->first, iter->second->Clone()));
}



//-------------------------------------------------------------------------------
// Structures
//-------------------------------------------------------------------------------

//
// Construct and initialize the structure of values
//
StructureRValue::StructureRValue(const StructureType& structuretype, IDType structuretypeid)
	: RValue(EpochVariableType_Structure),
	  StructureTypeID(structuretypeid)
{
}

//
// Copy a structure rvalue from an existing rvalue
//
StructureRValue::StructureRValue(const StructureRValue& rval)
	: RValue(EpochVariableType_Structure),
	  StructureTypeID(rval.StructureTypeID)
{
	CopyFrom(rval);
}

//
// Destruct and clean up the structure of values
//
StructureRValue::~StructureRValue()
{
	Clean();
}

//
// Clean up the structure
//
void StructureRValue::Clean()
{
	for(std::map<std::wstring, RValue*>::iterator iter = MemberData.begin(); iter != MemberData.end(); ++iter)
		delete iter->second;

	MemberData.clear();
}

//
// Copy from another structure
//
void StructureRValue::CopyFrom(const StructureRValue& rhs)
{
	StructureTypeID = rhs.StructureTypeID;

	for(std::map<std::wstring, RValue*>::const_iterator iter = rhs.MemberData.begin(); iter != rhs.MemberData.end(); ++iter)
		MemberData.insert(std::make_pair(iter->first, iter->second->Clone()));
}


//
// Add a member to the structure
//
// Call this function with EXTREME care; don't use it on existing variables,
// but rather only on variables that are being set up/bound to storage. Any
// other usage will actually change the underlying type of the structure, with
// unpredictable but likely nasty results.
//
void StructureRValue::AddMember(const std::wstring& membername, RValuePtr rval)
{
	if(MemberData.find(membername) != MemberData.end())
		throw ExecutionException("Duplicate member name - structure member names must all be different");

	MemberData[membername] = rval.release();
}


//
// Retrieve the value of one of the structure members
//
RValuePtr StructureRValue::GetValue(const std::wstring& membername) const
{
	std::map<std::wstring, RValue*>::const_iterator iter = MemberData.find(membername);
	if(iter == MemberData.end())
		throw ExecutionException("Specified identifier is not a member of this structure type");

	return RValuePtr(iter->second->Clone());
}

//
// Compare two structures to see if they are equal
//
bool StructureRValue::VirtualComparator(const RValue& rhs) const
{
	if(rhs.GetType() != EpochVariableType_Structure)
		return false;

	const StructureRValue& other = rhs.CastTo<StructureRValue>();
	if(other.StructureTypeID != StructureTypeID)
		return false;

	if(other.MemberData.size() != MemberData.size())
		return false;

	for(std::map<std::wstring, RValue*>::const_iterator iter = MemberData.begin(); iter != MemberData.end(); ++iter)
	{
		std::map<std::wstring, RValue*>::const_iterator otheriter = other.MemberData.find(iter->first);
		if(otheriter == other.MemberData.end())
			return false;

		if(iter->second->GetType() != otheriter->second->GetType())
			return false;

		switch(iter->second->GetType())
		{
		case EpochVariableType_Integer:
			if(iter->second->CastTo<IntegerRValue>().GetValue() != otheriter->second->CastTo<IntegerRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Integer16:
			if(iter->second->CastTo<Integer16RValue>().GetValue() != otheriter->second->CastTo<Integer16RValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Real:
			if(iter->second->CastTo<RealRValue>().GetValue() != otheriter->second->CastTo<RealRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Boolean:
			if(iter->second->CastTo<BooleanRValue>().GetValue() != otheriter->second->CastTo<BooleanRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_String:
			if(iter->second->CastTo<StringRValue>().GetValue() != otheriter->second->CastTo<StringRValue>().GetValue())
				return false;
			break;
		case EpochVariableType_Tuple:
			if(!iter->second->CastTo<TupleRValue>().VirtualComparator(*otheriter->second))
				return false;
			break;
		case EpochVariableType_Structure:
			if(!iter->second->CastTo<TupleRValue>().VirtualComparator(*otheriter->second))
				return false;
			break;

		default:
			throw NotImplementedException("Cannot compare structure members of this type");
		}
	}

	return true;
}



//-------------------------------------------------------------------------------
// Lists
//-------------------------------------------------------------------------------

//
// Destruct and clean up a list r-value
//
ListRValue::~ListRValue()
{
	Clean();
}

//
// Release all contained r-values and associated memory
//
void ListRValue::Clean()
{
	for(std::vector<RValue*>::iterator iter = Elements.begin(); iter != Elements.end(); ++iter)
		delete *iter;

	Elements.clear();
}

//
// Deep copy from another list r-value
//
void ListRValue::CopyFrom(const ListRValue& rhs)
{
	for(std::vector<RValue*>::const_iterator iter = rhs.Elements.begin(); iter != rhs.Elements.end(); ++iter)
		Elements.push_back((*iter)->Clone());
}

//
// Append an element to the list
//
void ListRValue::AddElement(RValue* element)
{
	Elements.push_back(element);
}

//
// Compare two lists for equality
//
bool ListRValue::VirtualComparator(const RValue& rhs) const
{
	if(rhs.GetType() != EpochVariableType_List)
		return false;

	const ListRValue& rhsvalue = rhs.CastTo<ListRValue>();
	if(rhsvalue.ElementType != ElementType)
		return false;

	if(rhsvalue.Elements.size() != Elements.size())
		return false;

	for(size_t i = 0; i < Elements.size(); ++i)
	{
		if(!Elements[i]->VirtualComparator(*rhsvalue.Elements[i]))
			return false;
	}

	return true;
}


//-------------------------------------------------------------------------------
// Buffers
//-------------------------------------------------------------------------------

//
// Destruct and clean up the byte buffer r-value
//
BufferRValue::~BufferRValue()
{
	Clean();
}

//
// Release the handle to the associated buffer (this will allow the garbage collector to free the buffer)
//
void BufferRValue::Clean()
{
	BufferHandle = 0;
}

//
// Copy from another buffer. We simply copy the buffer's handle.
// Note that this means that the r-value is bound to all future
// changes of that buffer!
//
void BufferRValue::CopyFrom(const BufferRValue& rhs)
{
	BufferHandle = rhs.BufferHandle;
}

//
// Determine if two buffers are equal (simple byte-wise comparison)
//
bool BufferRValue::VirtualComparator(const RValue& rhs) const
{
	if(rhs.GetType() != EpochVariableType_Buffer)
		return false;

	const BufferRValue& rhsvalue = rhs.CastTo<BufferRValue>();

	if(BufferHandle == rhsvalue.BufferHandle)
		return true;

	if(BufferVariable::Pool.Get(BufferHandle).Size != BufferVariable::Pool.Get(rhsvalue.BufferHandle).Size)
		return false;

	return (memcmp(BufferVariable::Pool.Get(BufferHandle).Buffer, BufferVariable::Pool.Get(rhsvalue.BufferHandle).Buffer, BufferVariable::Pool.Get(BufferHandle).Size) == 0);
}

