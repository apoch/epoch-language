//
// The Epoch Language Project
// FUGUE Virtual Machine
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

	// Constants
	extern const wchar_t* ConsoleMutexName;
	extern const wchar_t* MarshallingMutexName;


	// Thread forking
	void Create(const std::wstring& name, ThreadFuncPtr func, VM::Block* codeblock);
	void Create(const std::wstring& name, ThreadFuncPtr func, VM::Future* boundfuture, VM::Operation* op);

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

	// Thread manager setup/teardown
	void Init();
	void Shutdown();


	//
	// RAII wrapper of a system mutex. The named mutex must
	// be created separately prior to use of the AutoMutex.
	//
	class AutoMutex
	{
	// Construction and destruction
	public:
		explicit AutoMutex(const std::wstring& name);
		~AutoMutex();

	// Internal tracking
	private:
		HANDLE MutexHandle;
	};


	//
	// RAII wrapper of a special synchronization counter.
	// This counter is effectively an inverse semaphore;
	// when its count is 0, it is "unlocked", and when
	// the count is greater than 0, is is "locked."
	//
	class SyncCounter
	{
	// Construction and destruction
	public:
		explicit SyncCounter(unsigned* pcounter, HANDLE tripevent);
		~SyncCounter();

	// Internal tracking
	private:
		unsigned* PointerToCounter;
		HANDLE TripEvent;
	};


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
