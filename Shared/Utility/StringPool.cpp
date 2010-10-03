//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for pooling a set of strings and managing IDs for each entry
//

#include "pch.h"

#include "Utility/StringPool.h"


//
// Construct and initialize the string pool
//
StringPoolManager::StringPoolManager()
	: CurrentPooledStringHandle(0)
{
}


//
// Add a string to the pool, disallowing duplicate entries
//
// If an entry with matching content is found in the pool, its handle is returned.
// Otherwise, a newly allocated handle is returned.
//
StringHandle StringPoolManager::Pool(const std::wstring& stringdata)
{
	Threads::CriticalSection::Auto lock(CritSec);

	for(std::map<StringHandle, std::wstring>::const_iterator iter = PooledStrings.begin(); iter != PooledStrings.end(); ++iter)
	{
		if(iter->second == stringdata)
			return iter->first;
	}

	StringHandle handle = ++CurrentPooledStringHandle;
	PooledStrings.insert(std::make_pair(handle, stringdata));
	return handle;
}

//
// Assign the given handle to a string entry
//
// Replacing an existing string entry with a different string value is not permitted.
//
void StringPoolManager::Pool(StringHandle handle, const std::wstring& stringdata)
{
	Threads::CriticalSection::Auto lock(CritSec);

	std::pair<std::map<StringHandle, std::wstring>::iterator, bool> ret = PooledStrings.insert(std::make_pair(handle, stringdata));
	if(!ret.second && ret.first->second != stringdata)
		throw RecoverableException("Tried to replace a pooled string with a different value!");
	else if(CurrentPooledStringHandle < handle)
		CurrentPooledStringHandle = handle;
}

//
// Retrieve the string pooled with the given handle
//
const std::wstring& StringPoolManager::GetPooledString(StringHandle handle) const
{
	Threads::CriticalSection::Auto lock(CritSec);

	std::map<StringHandle, std::wstring>::const_iterator iter = PooledStrings.find(handle);
	if(iter == PooledStrings.end())
		throw RecoverableException("String handle does not correspond to any previously pooled string");

	return iter->second;
}
