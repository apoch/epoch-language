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

#include <fstream>
#include <iostream>

using namespace VM;


unsigned Program::ProgramInstances = 0;
Program* RunningProgram = NULL;


//
// Construct a program and initialize it
//
Program::Program() :
	GlobalInitBlock(NULL),
	GlobalStorageSpace(NULL),
	ActivatedGlobalScope(NULL)
{
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

	RunningProgram = this;
}

//
// Destruct and clean up a program wrapper
//
Program::~Program()
{
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

	std::auto_ptr<ActivatedScope> scope(new ActivatedScope(GlobalScope));
	ActivatedGlobalScope = scope.get();

	if(GlobalInitBlock)
	{
		delete GlobalStorageSpace;
		GlobalStorageSpace = new HeapStorage;
		FlowControlResult ignored = FLOWCONTROL_NORMAL;
		GlobalInitBlock->ExecuteBlock(*scope, Stack, ignored, GlobalStorageSpace);
	}

	RValuePtr ret(GlobalScope.GetFunction(L"entrypoint")->Invoke(Stack, *scope));

	Threads::WaitForThreadsToFinish();

	ActivatedGlobalScope = NULL;

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
// Global access to the running program
//
Program* VM::GetRunningProgram()
{
	return RunningProgram;
}
