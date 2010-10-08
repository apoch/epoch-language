//
// The Epoch Language Project
// Epoch Standard Library
//
// String pool management for flow control library
//

#include "pch.h"

#include "Library Functionality/Flow Control/StringPooling.h"

#include "Utility/StringPool.h"


void FlowControl::RegisterStrings(StringPoolManager& stringpool)
{
	stringpool.Pool(L"if");
	stringpool.Pool(L"elseif");
	stringpool.Pool(L"else");

	stringpool.Pool(L"while");
}
