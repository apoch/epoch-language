// EpochRT.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>

#include "StringPool.h"


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

extern "C" void ERT_print(const char* out)
{
	std::cout << out << std::endl;
}

extern "C" const char* ERT_string_concat(const char* s1, const char* s2)
{
	return StringPool.AllocConcat(s1, s2);
}

