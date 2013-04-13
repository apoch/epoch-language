//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Implementations for accessing global execution contexts
//

#include "pch.h"

#include "Runtime/GlobalContext.h"


using namespace Runtime;


namespace
{

	ExecutionContext* GlobalContext = NULL;

}


void Runtime::SetThreadContext(ExecutionContext* context)
{
	// This will eventually use thread-local storage once threading is reimplemented
	if(GlobalContext && context)
		throw FatalException("Cannot replace global execution context");

	GlobalContext = context;
}

ExecutionContext* Runtime::GetThreadContext()
{
	return GlobalContext;
}


