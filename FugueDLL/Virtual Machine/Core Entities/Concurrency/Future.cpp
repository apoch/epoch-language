//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class for futures
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Concurrency/Future.h"
#include "Virtual Machine/Core Entities/Operation.h"

#include "Utility/Memory/Stack.h"


using namespace VM;


//
// Construct and initialize a future wrapper
//
Future::Future(VM::OperationPtr op)
	: Op(op),
	  Result(NULL)
{
	std::wostringstream id;
	id << L"Epoch Future " << this;

	CompletionEventHandle = ::CreateEvent(NULL, FALSE, FALSE, id.str().c_str());
}

//
// Destruct and clean up a future wrapper
//
Future::~Future()
{
	::CloseHandle(CompletionEventHandle);
}


//
// Retrieve a future's value, blocking on the completion of the calculation if necessary
//
RValuePtr Future::GetValue() const
{
	::WaitForSingleObject(CompletionEventHandle, INFINITE);
	return RValuePtr(Result->Clone());
}

//
// Retrieve the type of data computed by the future
//
EpochVariableTypeID Future::GetType(const ScopeDescription& scope) const
{
	return Op->GetType(scope);
}

//
// Assign a result to the future for later retrieval, unblocking
// any threads that are currently waiting on the future.
//
void Future::SetResult(RValuePtr value)
{
	Result.reset(value.release());
	::SetEvent(CompletionEventHandle);
}
