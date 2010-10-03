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

#include <map>
#include <string>


class StringPoolManager
{
// Construction
public:
	StringPoolManager();

// Pooling interface
public:
	StringHandle Pool(const std::wstring& stringdata);
	void Pool(StringHandle handle, const std::wstring& stringdata);

	const std::wstring& GetPooledString(StringHandle handle) const;

// Direct access to pool, primarily for serialization purposes
public:
	const std::map<StringHandle, std::wstring>& GetInternalPool() const
	{ return PooledStrings; }

// Internal tracking
private:
	StringHandle CurrentPooledStringHandle;
	std::map<StringHandle, std::wstring> PooledStrings;
	Threads::CriticalSection CritSec;
};

