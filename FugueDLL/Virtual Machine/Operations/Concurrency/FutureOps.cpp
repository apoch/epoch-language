//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with futures
//

#include "pch.h"

#include "Virtual Machine/Operations/Concurrency/FutureOps.h"

#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Concurrency/Future.h"

#include "Virtual Machine/Core Entities/Variables/StringVariable.h"

#include "Virtual Machine/Core Entities/Program.h"

#include "Virtual Machine/Thread Pooling/WorkItems.h"

#include "Utility/Threading/Threads.h"


using namespace VM;
using namespace VM::Operations;


// Prototypes
DWORD __stdcall ExecuteEpochFutureTask(void* info);


//
// Fork a task that computes a future, and start execution of code in the new context
//
void ForkFuture::ExecuteFast(ExecutionContext& context)
{
	std::wostringstream id;
	id << L"Epoch Future Task " << this;
	Future* future = context.Scope.GetFuture(VarName);

	if(UseThreadPool)
	{
		VM::StringVariable temp(context.Stack.GetCurrentTopOfStack());
		std::wstring poolname = temp.GetValue();
		context.Stack.Pop(temp.GetStorageSize());

		std::auto_ptr<Threads::PoolWorkItem> workitem(new FutureWorkItem(*future, context));
		context.RunningProgram.AddPoolWorkItem(poolname, id.str(), workitem);
		return;
	}

	Threads::Create(id.str(), ExecuteEpochFutureTask, future, future->GetNestedOperation(), &context.RunningProgram);
}

RValuePtr ForkFuture::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}


//
// Entry point stub for executing implicit tasks assigned to futures
//
DWORD __stdcall ExecuteEpochFutureTask(void* info)
{
	try
	{
		Threads::Enter(info);

		Threads::ThreadInfo* threadinfo = reinterpret_cast<Threads::ThreadInfo*>(info);
		Operation* op = threadinfo->OpPointer;

		StackSpace stack;

		FlowControlResult flowresult = FLOWCONTROL_NORMAL;
		std::auto_ptr<ScopeDescription> descriptor(new ScopeDescription);
		std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*descriptor));
		newscope->TaskOrigin = threadinfo->TaskOrigin;
		newscope->Enter(stack);
		RValuePtr ret(op->ExecuteAndStoreRValue(ExecutionContext(*threadinfo->RunningProgram, *newscope, stack, flowresult))->Clone());
		newscope->Exit(stack);

		threadinfo->BoundFuture->SetResult(ret);

		if(stack.GetAllocatedStack() != 0)
			throw InternalFailureException("A stack space leak was detected when exiting an Epoch task.");
	}
	catch(std::exception& ex)
	{
		::MessageBoxA(0, ex.what(), Strings::WindowTitle, MB_ICONERROR);
	}
	catch(...)
	{
		::MessageBoxA(0, "An unexpected error has occurred while executing an Epoch task.", Strings::WindowTitle, MB_ICONERROR);
	}

	Threads::Exit();
	return 0;
}

