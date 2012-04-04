//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for precompiling functions to native code
//

#include "pch.h"

#include "Library Functionality/Function Tags/Native.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


namespace
{

	//
	// Register a function tagged as wanting precompilation to native code
	//
	TagHelperReturn NativeHelper(StringHandle, const CompileTimeParameterVector&, bool isprepass)
	{
		TagHelperReturn ret;

		if(isprepass)
		{
			ret.MetaTag = L"native";
		}

		return ret;
	}

}


//
// Bind the library to an execution dispatch table
//
void FunctionTags::RegisterNativeTag(FunctionSignatureSet&, StringPoolManager&)
{
}

//
// Bind the library to a function metadata table
//
void FunctionTags::RegisterNativeTag(EpochFunctionPtr, FunctionInvocationTable&, StringPoolManager&)
{
}


//
// Bind the library's tag helpers to the compiler
//
void FunctionTags::RegisterNativeTagHelper(FunctionTagHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"native", &NativeHelper));
}

