#include "stdafx.h"
#include "StringPool.h"



ThreadStringPool::ThreadStringPool()
	: TraceFlag(0)
{
}

ThreadStringPool::~ThreadStringPool()
{
	for(auto& entry : Pool)
		delete entry.String;
}


const char* ThreadStringPool::AllocConcat(const char* s1, const char* s2)
{
	std::ostringstream concat;
	concat << s1;
	concat << s2;

	Pool.emplace_back(TraceFlag, new std::string(concat.str()));

	return Pool.back().String->c_str();
}


void ThreadStringPool::FreeUnusedEntries()
{
	uint32_t count = 0;
	uint32_t bit = TraceFlag;
	auto iter = std::remove_if(Pool.begin(), Pool.end(), [bit, &count](const PoolEntry& entry) {
		if(entry.TraceFlag != bit)
		{
			++count;
			delete entry.String;
			return true;
		}

		return false;
	});

	Pool.erase(iter, Pool.end());

	std::cout << "GC Freed " << count << " strings" << std::endl;
}


void ThreadStringPool::ToggleTraceBit()
{
	TraceFlag = 1 - TraceFlag;
}


void ThreadStringPool::MarkInUse(const char* p)
{
	for(auto& entry : Pool)
	{
		if(entry.String->c_str() == p)
		{
			entry.TraceFlag = TraceFlag;
			return;
		}
	}
}


