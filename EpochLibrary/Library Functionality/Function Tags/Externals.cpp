//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for external (DLL-based) functions
//

#include "pch.h"

#include "Library Functionality/Function Tags/Externals.h"
#include "Libraries/LibraryJIT.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Virtual Machine/Marshaling.h"


extern StringHandle ExternalHandle;
extern StringHandle ExternalJITHandle;


namespace
{

	//
	// Register a function tagged as an external
	//
	// In the prepass compilation phase, this simply attaches some metadata to the function entity,
	// which is emitted into the final compiled code byte stream. This metadata is used to track
	// the specific DLL and function that should be invoked by the marshaling system. In the final
	// compilation pass, we signal the compiler to automatically invoke the magic @@external code,
	// which takes care of actually marshaling the call to the external DLL. Note that it is not
	// required that the function return immediately after invoking @@external; it is permissible
	// to define a code body for the function which performs additional logic after the marshaled
	// call returns.
	//
	TagHelperReturn ExternalHelper(StringHandle, const CompileTimeParameterVector& compiletimeparams, bool isprepass)
	{
		TagHelperReturn ret;

		if(!isprepass)
			ret.InvokeRuntimeFunction = L"@@external";
		else
		{
			ret.MetaTag = L"external";
			ret.MetaTagData.push_back(compiletimeparams[0].StringPayload);
			ret.MetaTagData.push_back(compiletimeparams[1].StringPayload);

			if(compiletimeparams.size() >= 3)
				ret.MetaTagData.push_back(compiletimeparams[2].StringPayload);
		}

		return ret;
	}


	void ExternalJIT(JIT::JITContext& context, bool)
	{
		context.Generator->ExternalInvoke(context, context.FunctionAlias);
	}

}


//
// Bind the library to an execution dispatch table
//
void FunctionTags::RegisterExternalTag(FunctionSignatureSet& signatureset)
{
	AddToMapNoDupe(signatureset, std::make_pair(ExternalHandle, FunctionSignature()));
}

//
// Bind the library's tag helpers to the compiler
//
void FunctionTags::RegisterExternalTagHelper(FunctionTagHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"external", &ExternalHelper));
}


void FunctionTags::RegisterExternalTagJITTable(JIT::JITTable& jittable)
{
	AddToMapNoDupe(jittable.InvokeHelpers, std::make_pair(ExternalJITHandle, &ExternalJIT));
}

