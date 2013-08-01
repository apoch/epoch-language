//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for pooling a set of strings and managing IDs for each entry
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Threading/Synchronization.h"
#include "Utility/HandleAllocator.h"

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <string>


class StringPoolManager
{
// Construction
public:
	explicit StringPoolManager(bool fastreverse = false)
		: FastLookupEnabled(fastreverse),
		  GarbageTick(0)
	{ }

// Pooling interface
public:
	StringHandle Pool(const std::wstring& stringdata);
	StringHandle PoolFast(const std::wstring& stringdata);
	void Pool(StringHandle handle, const std::wstring& stringdata);

	const std::wstring& GetPooledString(StringHandle handle) const;
	StringHandle Find(const std::wstring& stringdata) const;

// Direct access to pool, primarily for serialization purposes
public:
	const boost::unordered_map<StringHandle, std::wstring>& GetInternalPool() const
	{ return PooledStrings; }

// Garbage collection interface
public:
	void GarbageCollect(const boost::unordered_set<StringHandle>& livehandles, const boost::unordered_set<StringHandle>& statichandles);

// Internal tracking
private:
	bool FastLookupEnabled;
	HandleAllocator<StringHandle> HandleAlloc;
	boost::unordered_map<StringHandle, std::wstring> PooledStrings;

// Public access to the critical section, for direct access purposes
public:
	Threads::CriticalSection CritSec;

// Garbage collection
public:
	unsigned GarbageTick;
	
#ifdef EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP
private:
	boost::unordered_map<std::wstring, StringHandle> ReverseLookupMap;
#endif
};

