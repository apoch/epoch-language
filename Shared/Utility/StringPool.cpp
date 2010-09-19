//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for pooling a set of strings and managing IDs for each entry
//

#include "pch.h"

#include "Utility/StringPool.h"


StringPoolManager::StringPoolManager()
	: CurrentPooledStringHandle(0)
{
}


StringHandle StringPoolManager::Pool(const std::wstring& stringdata)
{
	for(std::map<StringHandle, std::wstring>::const_iterator iter = PooledStrings.begin(); iter != PooledStrings.end(); ++iter)
	{
		if(iter->second == stringdata)
			return iter->first;
	}

	StringHandle handle = ++CurrentPooledStringHandle;
	PooledStrings.insert(std::make_pair(handle, stringdata));
	return handle;
}

void StringPoolManager::Pool(StringHandle handle, const std::wstring& stringdata)
{
	std::pair<std::map<StringHandle, std::wstring>::iterator, bool> ret = PooledStrings.insert(std::make_pair(handle, stringdata));
	if(!ret.second && ret.first->second != stringdata)
		throw RecoverableException("Tried to replace a pooled string with a different value!");
	else if(CurrentPooledStringHandle < handle)
		CurrentPooledStringHandle = handle;
}


const std::wstring& StringPoolManager::GetPooledString(StringHandle handle) const
{
	std::map<StringHandle, std::wstring>::const_iterator iter = PooledStrings.find(handle);
	if(iter == PooledStrings.end())
		throw RecoverableException("String handle does not correspond to any previously pooled string");

	return iter->second;
}
