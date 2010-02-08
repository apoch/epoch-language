//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations and auxiliary functions for asynchronous task support
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"
#include "Virtual Machine/Core Entities/Program.h"

#include "Virtual Machine/Operations/Concurrency/Tasks.h"
#include "Virtual Machine/Thread Pooling/WorkItems.h"
#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/Routines.inl"
#include "Virtual Machine/VMExceptions.h"

#include "Virtual Machine/SelfAware.inl"

#include "Parser/Parser State Machine/ParserState.h"

#include "Utility/Threading/Threads.h"
#include "Utility/Memory/Heap.h"
#include "Utility/Memory/Stack.h"
#include "Utility/Strings.h"

#include "Validator/Validator.h"

#include "Serialization/SerializationTraverser.h"


using namespace VM;
using namespace VM::Operations;


// Prototypes
DWORD __stdcall ExecuteEpochTask(void* info);


//
// Destruct and clean up a task forking operation
//
ForkTask::~ForkTask()
{
	delete CodeBlock;
}

//
// Fork a task and start execution in the new context
//
void ForkTask::ExecuteFast(ExecutionContext& context)
{
	StringVariable temp(context.Stack.GetCurrentTopOfStack());
	std::wstring taskname = temp.GetValue();
	context.Stack.Pop(temp.GetStorageSize());
	Threads::Create(taskname, ExecuteEpochTask, CodeBlock, &context.RunningProgram);
}

RValuePtr ForkTask::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}

template <typename TraverserT>
void ForkTask::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	traverser.EnterTask();
	if(CodeBlock)
		CodeBlock->Traverse(traverser);
	traverser.ExitTask();
}

void ForkTask::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void ForkTask::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


//
// Destruct and clean up a thread forking operation
//
ForkThread::~ForkThread()
{
	delete CodeBlock;
}

//
// Fork a thread and start execution in the new context
//
void ForkThread::ExecuteFast(ExecutionContext& context)
{
	std::wstring threadname, poolname;

	{
		StringVariable temp(context.Stack.GetCurrentTopOfStack());
		poolname = temp.GetValue();
		context.Stack.Pop(temp.GetStorageSize());
	}

	{
		StringVariable temp(context.Stack.GetCurrentTopOfStack());
		threadname = temp.GetValue();
		context.Stack.Pop(temp.GetStorageSize());
	}

	std::auto_ptr<Threads::PoolWorkItem> workitem(new ForkThreadWorkItem(*CodeBlock, context));
	context.RunningProgram.AddPoolWorkItem(poolname, threadname, workitem);
}

RValuePtr ForkThread::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}

template <typename TraverserT>
void ForkThread::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	traverser.EnterThread();
	if(CodeBlock)
		CodeBlock->Traverse(traverser);
	traverser.ExitThread();
}

void ForkThread::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void ForkThread::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


void CreateThreadPool::ExecuteFast(ExecutionContext& context)
{
	std::wstring poolname;
	Integer32 numthreads;

	{
		IntegerVariable temp(context.Stack.GetCurrentTopOfStack());
		numthreads = temp.GetValue();
		context.Stack.Pop(temp.GetStorageSize());
	}

	{
		StringVariable temp(context.Stack.GetCurrentTopOfStack());
		poolname = temp.GetValue();
		context.Stack.Pop(temp.GetStorageSize());
	}

	context.RunningProgram.CreateThreadPool(poolname, numthreads);
}

RValuePtr CreateThreadPool::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}


//
// Entry point stub for forked Epoch task threads
//
DWORD __stdcall ExecuteEpochTask(void* info)
{
	try
	{
		Threads::Enter(info);

		Threads::ThreadInfo* threadinfo = reinterpret_cast<Threads::ThreadInfo*>(info);
		Block* codeblock = threadinfo->CodeBlock;

		StackSpace stack;

		FlowControlResult flowresult = FLOWCONTROL_NORMAL;
		std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*codeblock->GetBoundScope(), threadinfo->RunningProgram->GetActivatedGlobalScope()));
		newscope->TaskOrigin = threadinfo->TaskOrigin;
		codeblock->ExecuteBlock(ExecutionContext(*threadinfo->RunningProgram, *newscope, stack, flowresult), NULL);
		newscope->Exit(stack);

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



