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
void ForkTask::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	StringVariable temp(stack.GetCurrentTopOfStack());
	std::wstring taskname = temp.GetValue();
	stack.Pop(temp.GetStorageSize());
	Threads::Create(taskname, ExecuteEpochTask, CodeBlock);
}

RValuePtr ForkTask::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
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
void ForkThread::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	std::wstring threadname, poolname;

	{
		StringVariable temp(stack.GetCurrentTopOfStack());
		poolname = temp.GetValue();
		stack.Pop(temp.GetStorageSize());
	}

	{
		StringVariable temp(stack.GetCurrentTopOfStack());
		threadname = temp.GetValue();
		stack.Pop(temp.GetStorageSize());
	}

	// TODO - actually place this code on the work queue of the appropriate thread pool
}

RValuePtr ForkThread::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
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


void CreateThreadPool::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	std::wstring threadname;
	Integer32 numthreads;

	{
		IntegerVariable temp(stack.GetCurrentTopOfStack());
		numthreads = temp.GetValue();
		stack.Pop(temp.GetStorageSize());
	}

	{
		StringVariable temp(stack.GetCurrentTopOfStack());
		threadname = temp.GetValue();
		stack.Pop(temp.GetStorageSize());
	}

	// TODO - create thread pool
}

RValuePtr CreateThreadPool::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
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

		std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*codeblock->GetBoundScope(), VM::GetRunningProgram()->GetActivatedGlobalScope()));
		newscope->TaskOrigin = threadinfo->TaskOrigin;
		codeblock->ExecuteBlock(*newscope, stack, flowresult, NULL);
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



