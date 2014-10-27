// EpochRT.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>


extern "C" void ERT_print(const char* out)
{
	std::cout << out << std::endl;
}

