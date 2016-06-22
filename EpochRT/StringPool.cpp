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

