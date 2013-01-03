//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for pooling a set of strings and managing IDs for each entry
//

#include "pch.h"

#include "Utility/StringPool.h"
#include "Utility/EraseDeadHandles.h"


//
// Add a string to the pool, disallowing duplicate entries
//
// If an entry with matching content is found in the pool, its handle is returned.
// Otherwise, a newly allocated handle is returned.
//
StringHandle StringPoolManager::Pool(const std::wstring& stringdata)
{
	Threads::CriticalSection::Auto lock(CritSec);

#ifdef EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP
	if(FastLookupEnabled)
	{
		boost::unordered_map<std::wstring, StringHandle>::const_iterator iter = ReverseLookupMap.find(stringdata);
		if(iter != ReverseLookupMap.end())
			return iter->second;
	}
	else
#endif
	{
		for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = PooledStrings.begin(); iter != PooledStrings.end(); ++iter)
		{
			if(iter->second == stringdata)
				return iter->first;
		}
	}

	return PoolFast(stringdata);
}

//
// Add a string to the pool, permitting duplicate entries
//
// The entry is added to the pool regardless of content, and its newly allocated handle
// is returned as quickly as possible.
//
StringHandle StringPoolManager::PoolFast(const std::wstring& stringdata)
{
	Threads::CriticalSection::Auto lock(CritSec);

	StringHandle handle = HandleAlloc.AllocateHandle(PooledStrings);
	PooledStrings.insert(std::make_pair(handle, stringdata));

#ifdef EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP
	if(FastLookupEnabled)
		ReverseLookupMap.insert(std::make_pair(stringdata, handle));
#endif

	return handle;
}

//
// Assign the given handle to a string entry
//
// Replacing an existing string entry with a different string value is not permitted.
//
void StringPoolManager::Pool(StringHandle handle, const std::wstring& stringdata)
{
#ifdef EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP
	if(FastLookupEnabled)
		throw FatalException("Cannot pool strings to known IDs when using fast reverse lookup!");
#endif

	Threads::CriticalSection::Auto lock(CritSec);

	std::pair<boost::unordered_map<StringHandle, std::wstring>::iterator, bool> ret = PooledStrings.insert(std::make_pair(handle, stringdata));
	if(!ret.second && ret.first->second != stringdata)
		throw RecoverableException("Tried to replace a pooled string with a different value!");

	HandleAlloc.BumpHandle(handle);
}

//
// Retrieve the string pooled with the given handle
//
const std::wstring& StringPoolManager::GetPooledString(StringHandle handle) const
{
	Threads::CriticalSection::Auto lock(CritSec);

	boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = PooledStrings.find(handle);
	if(iter == PooledStrings.end())
		throw RecoverableException("String handle does not correspond to any previously pooled string");

	return iter->second;
}

//
// Find a particular string, assuming it has already been pooled
//
StringHandle StringPoolManager::Find(const std::wstring& stringdata) const
{
	Threads::CriticalSection::Auto lock(CritSec);

#ifdef EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP
	if(FastLookupEnabled)
	{
		boost::unordered_map<std::wstring, StringHandle>::const_iterator iter = ReverseLookupMap.find(stringdata);
		if(iter != ReverseLookupMap.end())
			return iter->second;
	}
	else
#endif
	{
		for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = PooledStrings.begin(); iter != PooledStrings.end(); ++iter)
		{
			if(iter->second == stringdata)
				return iter->first;
		}
	}

	throw RecoverableException("String not pooled yet");
}

//
// Discard all handles NOT in the given set of live handles
//
void StringPoolManager::GarbageCollect(const boost::unordered_set<StringHandle>& livehandles)
{
#ifdef EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP
	if(FastLookupEnabled)
		throw FatalException("Cannot garbage collect when using fast reverse lookup!");
#endif

	Threads::CriticalSection::Auto lock(CritSec);
	EraseDeadHandles(PooledStrings, livehandles);
}
