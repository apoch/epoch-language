// EpochRT.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>


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

