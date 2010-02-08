//
// The Epoch Language Project
// Shared Library Code
//
// Platform-dependent threading wrappers
//

#pragma once


// Forward declarations
class HeapStorage;
namespace VM
{
	class Block;
	class Operation;
	class Future;
	class ActivatedScope;
	class Program;
}

// Dependencies
#include "Utility/Memory/Heap.h"

#include "Utility/Threading/Mailbox.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"


namespace Threads
{
	// Forward declarations
	struct MessageInfo;
	struct ThreadInfo;

	// Handy type shortcuts
	typedef DWORD (__stdcall *ThreadFuncPtr)(void* param);


	// Thread forking
	void Create(const std::wstring& name, ThreadFuncPtr func, VM::Block* codeblock, VM::Program* runningprogram);
	void Create(const std::wstring& name, ThreadFuncPtr func, VM::Future* boundfuture, VM::Operation* op, VM::Program* runningprogram);

	// Helpers for setup/teardown of threads
	void Enter(void* info);
	void Exit();
	void WaitForThreadsToFinish();
	
	// Message passing
	void SendEvent(const std::wstring& threadname, const std::wstring& eventname, const std::list<VM::EpochVariableTypeID>& payloadtypes, HeapStorage* storageblock);
	Threads::MessageInfo* WaitForEvent();

	// Thread info access
	const ThreadInfo& GetInfoForThisThread();
	std::wstring GetThreadNameGivenID(TaskHandle id);
	DWORD GetTLSIndex();

	// Thread manager setup/teardown
	void Init();
	void Shutdown();


	//
	// Descriptor of a running thread
	//
	struct ThreadInfo
	{
		union
		{
			VM::Block* CodeBlock;
			VM::Operation* OpPointer;
		};
		VM::Future* BoundFuture;
		VM::Program* RunningProgram;
		DWORD HandleToSelf;
		DWORD TaskOrigin;
		HANDLE LocalHeapHandle;
		HANDLE MessageEvent;
		LocklessMailbox<MessageInfo>* Mailbox;
	};

	//
	// Message payload structure for inter-thread communication
	//
	struct MessageInfo
	{
		std::wstring MessageName;
		std::list<VM::EpochVariableTypeID> PayloadTypes;
		DWORD Origin;
		HeapStorage* StorageBlock;

		~MessageInfo()
		{ delete StorageBlock; }
	};

}
