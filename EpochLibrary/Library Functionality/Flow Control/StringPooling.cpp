//
// The Epoch Language Project
// Epoch Standard Library
//
// String pool management for flow control library
//

#include "pch.h"

#include "Library Functionality/Flow Control/StringPooling.h"

#include "Utility/StringPool.h"


//
// Register all the strings used by the flow control library
//
// This is done here to ensure that strings are always registered in a consistent
// order, which avoids issues with handle collisions when pooling strings in the VM.
//
void FlowControl::RegisterStrings(StringPoolManager& stringpool)
{
	stringpool.Pool(L"if");
	stringpool.Pool(L"elseif");
	stringpool.Pool(L"else");

	stringpool.Pool(L"while");
	stringpool.Pool(L"do");
}
