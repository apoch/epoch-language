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

	DWORD TlsIndex = TLS_OUT_OF_INDEXES;

}


void Runtime::SetThreadContext(ExecutionContext* context)
{
	if(TlsIndex == TLS_OUT_OF_INDEXES)
		TlsIndex = ::TlsAlloc();

	void* currentvalue = ::TlsGetValue(TlsIndex);

	if(currentvalue && context)
		throw FatalException("Cannot replace thread's execution context");

	::TlsSetValue(TlsIndex, context);
}

ExecutionContext* Runtime::GetThreadContext()
{
	if(TlsIndex == TLS_OUT_OF_INDEXES)
		throw FatalException("Cannot retrieve thread's execution context");

	return reinterpret_cast<ExecutionContext*>(::TlsGetValue(TlsIndex));
}


