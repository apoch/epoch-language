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
#include <set>


struct PooledString
{
	std::wstring Data;
	bool InUse;
};


class StringPoolManager
{
// Construction
public:
	explicit StringPoolManager(bool fastreverse = false)
		: FastLookupEnabled(fastreverse),
		  GarbageTick(0)
	{
	}

// Pooling interface
public:
	StringHandle Pool(const std::wstring& stringdata);
	StringHandle PoolFast(const std::wstring& stringdata);
	StringHandle PoolFast(const wchar_t* stringdata);
	void Pool(StringHandle handle, const std::wstring& stringdata);

	const std::wstring& GetPooledString(StringHandle handle) const;
	StringHandle Find(const std::wstring& stringdata) const;

// Direct access to pool, primarily for serialization purposes
public:
	const std::map<StringHandle, PooledString>& GetInternalPool() const
	{ return PooledStrings; }

// Garbage collection interface
public:
	void UnmarkAllStrings();
	void MarkStringActive(StringHandle h);
	void GarbageCollect(const boost::unordered_set<StringHandle>& statichandles);

// Internal tracking
private:
	bool FastLookupEnabled;
	HandleAllocator<StringHandle> HandleAlloc;
	std::map<StringHandle, PooledString> PooledStrings;

// Garbage collection
private:
	unsigned GarbageTick;

public:
	unsigned GetGarbageTick() const
	{ return GarbageTick; }

	void ResetGarbageTick()
	{ GarbageTick = 0; }
	
#ifdef EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP
private:
	boost::unordered_map<std::wstring, StringHandle> ReverseLookupMap;
#endif
};

