//
// The Epoch Language Project
// Epoch Standard Library
//
// String pool management for flow control library
//

#include "pch.h"

#include "Library Functionality/Flow Control/StringPooling.h"

#include "Utility/StringPool.h"


extern StringHandle IfHandle;
extern StringHandle ElseIfHandle;
extern StringHandle ElseHandle;

extern StringHandle WhileHandle;
extern StringHandle DoHandle;

extern StringHandle ReturnHandle;


//
// Register all the strings used by the flow control library
//
// This is done here to ensure that strings are always registered in a consistent
// order, which avoids issues with handle collisions when pooling strings in the VM.
//
void FlowControl::PoolStrings(StringPoolManager& stringpool)
{
	IfHandle = stringpool.Pool(L"if");
	ElseIfHandle = stringpool.Pool(L"elseif");
	ElseHandle = stringpool.Pool(L"else");

	WhileHandle = stringpool.Pool(L"while");
	DoHandle = stringpool.Pool(L"do");

	ReturnHandle = stringpool.Pool(L"return");
}
