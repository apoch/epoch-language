#include "stdafx.h"
#include "StringPool.h"



ThreadStringPool::~ThreadStringPool()
{
	for(auto * entry : Pool)
		delete entry;
}


const char* ThreadStringPool::AllocConcat(const char* s1, const char* s2)
{
	std::ostringstream concat;
	concat << s1;
	concat << s2;

	Pool.emplace_back(new std::string(concat.str()));

	return Pool.back()->c_str();
}


void ThreadStringPool::FreeUnusedEntries()
{
	// TODO - mark used entries and don't free them

	auto iter = std::remove_if(Pool.begin(), Pool.end(), [](const std::string* entry) {
		delete entry;
		return true;
	});

	Pool.erase(iter, Pool.end());
}

