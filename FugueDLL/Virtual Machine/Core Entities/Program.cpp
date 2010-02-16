//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class encapsulating a complete Epoch program.
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Types/Tuple.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"

#include "Language Extensions/ExtensionCatalog.h"

#include "Marshalling/Callback.h"

#include "Utility/Strings.h"
#include "Utility/Memory/Heap.h"

#include "Utility/Threading/Threads.h"

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <fstream>
#include <iostream>

using namespace VM;


unsigned Program::ProgramInstances = 0;


//
// Construct a program and initialize it
//
Program::Program() :
	GlobalInitBlock(NULL),
	GlobalStorageSpace(NULL),
	ActivatedGlobalScope(NULL),
	FlagsUsesConsole(false)
{
	// TODO - remove single-program-per-process gibberish

	// This is kind of lame and inelegant; there shouldn't be a limit on the program
	// instances active because everything should be able to run independently.
	// Eventually it would be nice to refactor/adjust things to eliminate the one
	// instance requirement.
	if(ProgramInstances > 0)
		throw Exception("Cannot instantiate multiple programs in the same process!");

	++ProgramInstances;
	TupleTrackerClass::ResetSharedData();
	StructureTrackerClass::ResetSharedData();
	Marshalling::Clean();
	StringVariable::EmptyPool();
}

//
// Destruct and clean up a program wrapper
//
Program::~Program()
{
	delete ActivatedGlobalScope;
	delete GlobalInitBlock;
	delete GlobalStorageSpace;
	--ProgramInstances;
}


//
// Execute an Epoch program
//
RValuePtr Program::Execute()
{
	Extensions::PrepareForExecution();

	delete ActivatedGlobalScope;
	ActivatedGlobalScope = new ActivatedScope(GlobalScope);

	if(GlobalInitBlock)
	{
		delete GlobalStorageSpace;
		GlobalStorageSpace = new HeapStorage;
		FlowControlResult ignored = FLOWCONTROL_NORMAL;
		GlobalInitBlock->ExecuteBlock(ExecutionContext(*this, *ActivatedGlobalScope, Stack, ignored), GlobalStorageSpace);
	}

	FlowControlResult flowresult = FLOWCONTROL_NORMAL;
	RValuePtr ret(GlobalScope.GetFunction(L"entrypoint")->Invoke(ExecutionContext(*this, *ActivatedGlobalScope, Stack, flowresult)));

	Threads::WaitForThreadsToFinish();

	size_t allocatedstack = Stack.GetAllocatedStack();
	if(allocatedstack != 0)
		throw ExecutionException("Stack leakage has occurred!");
	
	return ret;
}

//
// Pool a static string value in a central location
//
// This strategy allows us to keep a guaranteed single copy of any
// string identifier/etc. used by the program, and safely work with
// references to that single copy.
//
const std::wstring& Program::PoolStaticString(const std::wstring& str)
{
	std::set<std::wstring>::const_iterator iter = StaticStringPool.insert(str).first;
	return *iter;
}

//
// Set the block of code that should run before the entrypoint; used for global data
//
// Use sparingly - see CreateGlobalInitBlock() for details
//
void Program::ReplaceGlobalInitBlock(VM::Block* block)
{
	delete GlobalInitBlock;
	GlobalInitBlock = block;
	GlobalInitBlock->BindToScope(&GetGlobalScope());
}

//
// Retrieve the block of code that runs before the entrypoint, creating a new block if needed
//
// Client code should prefer to use this function instead of ReplaceGlobalInitBlock, so that
// if multiple areas of the code need to work on the global init block, they can do so without
// overwriting each other's changes.
//
VM::Block& Program::CreateGlobalInitBlock()
{
	if(!GlobalInitBlock)
		ReplaceGlobalInitBlock(new VM::Block(false));

	return *GlobalInitBlock;
}


//
// Create a thread pool of worker threads.
//
// The created pool can later be referenced by the name provided.
//
void Program::CreateThreadPool(const std::wstring& poolname, unsigned numthreads)
{
	ThreadPools.CreateNamedPool(poolname, numthreads, this);
}

//
// Add a work item to a worker thread pool's work queue
//
void Program::AddPoolWorkItem(const std::wstring& poolname, const std::wstring& threadname, std::auto_ptr<Threads::PoolWorkItem> workitem)
{
	ThreadPools.GetNamedPool(poolname).AddWorkItem(threadname, workitem.release());
}

