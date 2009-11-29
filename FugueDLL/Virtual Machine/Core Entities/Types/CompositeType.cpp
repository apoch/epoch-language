//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Base class for all composite data types in Epoch
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Types/CompositeType.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"
#include "Virtual Machine/Types Management/TypeInfo.h"


using namespace VM;


//
// These should pretty much explain themselves.
//
void CompositeType::AddMember(const std::wstring& name, VM::EpochVariableTypeID type)
{
	MemberInfo info;
	info.Type = type;
	info.Offset = 0;
	info.TypeHint = 0;
	StorageSize += TypeInfo::GetStorageSize(type);

	MemberInfoMap.insert(std::make_pair(name, info));
	MemberOrder.push_back(name);
}

void CompositeType::AddMember(const std::wstring& name, const StructureType& type, IDType typehint)
{
	MemberInfo info;
	info.Type = EpochVariableType_Structure;
	info.Offset = 0;
	info.TypeHint = typehint;
	StorageSize += type.GetTotalSize();

	MemberInfoMap.insert(std::make_pair(name, info));
	MemberOrder.push_back(name);
}

void CompositeType::AddMember(const std::wstring& name, const TupleType& type, IDType typehint)
{
	MemberInfo info;
	info.Type = EpochVariableType_Tuple;
	info.Offset = 0;
	info.TypeHint = typehint;
	StorageSize += type.GetTotalSize();

	MemberInfoMap.insert(std::make_pair(name, info));
	MemberOrder.push_back(name);
}

void CompositeType::AddFunctionMember(const std::wstring& name, const std::wstring& hint)
{
	MemberInfo info;
	info.Type = EpochVariableType_Function;
	info.Offset = 0;
	info.TypeHint = 0;
	info.StringHint = hint;
	StorageSize += FunctionBinding::GetStorageSize();

	MemberInfoMap.insert(std::make_pair(name, info));
	MemberOrder.push_back(name);
}


//
// Retrieve the type of a given member
//
EpochVariableTypeID CompositeType::GetMemberType(const std::wstring& name) const
{
	std::map<std::wstring, MemberInfo>::const_iterator iter = MemberInfoMap.find(name);
	if(iter == MemberInfoMap.end())
		throw ExecutionException("The requested identifier does not match any members of the composite type");

	return iter->second.Type;
}

//
// Retrieve the type hint of a given member
// WARNING: this does not check for validity!
//
IDType CompositeType::GetMemberTypeHint(const std::wstring& name) const
{
	std::map<std::wstring, MemberInfo>::const_iterator iter = MemberInfoMap.find(name);
	if(iter == MemberInfoMap.end())
		throw ExecutionException("The requested identifier does not match any members of the composite type");

	return iter->second.TypeHint;
}

//
// Retrieve the string hint of a given member (string hints are used for function signatures)
// WARNING: this does not check for validity!
//
const std::wstring& CompositeType::GetMemberTypeHintString(const std::wstring& name) const
{
	std::map<std::wstring, MemberInfo>::const_iterator iter = MemberInfoMap.find(name);
	if(iter == MemberInfoMap.end())
		throw ExecutionException("The requested identifier does not match any members of the composite type");

	return iter->second.StringHint;
}

//
// Retrieve the offset, in bytes, of the given member from the start of the variable's storage space
//
size_t CompositeType::GetMemberOffset(const std::wstring& name) const
{
	std::map<std::wstring, MemberInfo>::const_iterator iter = MemberInfoMap.find(name);
	if(iter == MemberInfoMap.end())
		throw ExecutionException("The requested identifier does not match any members of the composite type");

#ifdef _DEBUG
	if(iter->second.Offset == 0)
		throw InternalFailureException("Composite type member offsets have not been computed!");
#endif

	return iter->second.Offset;
}


