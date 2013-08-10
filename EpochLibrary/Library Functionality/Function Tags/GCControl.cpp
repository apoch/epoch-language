//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for GC control
//

#include "pch.h"

#include "Library Functionality/Function Tags/GCControl.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


extern StringHandle NoGCHandle;


namespace
{

	TagHelperReturn NoGCHelper(StringHandle, const CompileTimeParameterVector&, bool isprepass)
	{
		TagHelperReturn ret;

		if(isprepass)
			ret.MetaTag = L"nogc";

		return ret;
	}

}


//
// Bind the library's tag helpers to the compiler
//
void FunctionTags::RegisterGCTagHelpers(FunctionTagHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"nogc", &NoGCHelper));
}

