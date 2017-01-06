// EpochRT.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>

#include "StringPool.h"
#include "GC.h"


// TODO - thread safety
ThreadStringPool StringPool;


extern "C" void ERT_assert(bool flag)
{
	// TODO - better handling here
	if(!flag)
		exit(0xffffffff);
}

extern "C" void ERT_passtest()
{
	std::cout << "TEST: pass" << std::endl;
}

extern "C" const char* ERT_string_concat(const char* s1, const char* s2)
{
	return StringPool.AllocConcat(s1, s2);
}

extern "C" bool ERT_string_compare(const char* s1, const char* s2)
{
	return (std::strcmp(s1, s2) == 0);
}

extern "C" const char* ERT_string_from_integer(int i)
{
	std::ostringstream convert;
	convert << i;

	return StringPool.Alloc(convert.str());
}

extern "C" void ERT_gc_init(unsigned segmentoffset)
{
	GC::Init(segmentoffset);
}

extern "C" void ERT_gc_collect_strings()
{
	GC::CollectStrings(&StringPool, _ReturnAddress());
}


extern "C" short ERT_integer16_from_integer(int in)
{
	return static_cast<short>(in);
}


extern "C" void ERT_print(const char* out)
{
	std::cout << out << std::endl;
}

