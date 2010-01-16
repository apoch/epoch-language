//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Platform-dependent threading wrappers
//
// Note that we use a fairly intricate system here to provide certain semantics
// during data read and write operations. In particular, we store a map that
// links thread names with their descriptor blocks (which contain information
// necessary to perform communication with the thread, etc.). Since this map
// is modified each time a thread starts or exits, it is protected by a mutex
// in the thread start/stop functions, and a set of cooperative operations for
// functions which need to read the map but will never write to it.
//
// Although we are effectively locking on access to the lookup map, there is
// a system established to allow readers to examine the map without locking
// against each other. In other words, anyone who simply wants to read from
// the map can do so without locking, unless a thread start/stop operation is
// pending, in which case the read must wait for the write to be completed.
//
// The message send and receive operations both read from the map but do not
// write to it. Therefore, these operations are guarded by a Windows Event.
// This event is signalled during normal operation, and set to a non-signalled
// state when a thread start/stop is being performed. Therefore, the read
// operations will wait until the start/stop is completed before executing.
//
// The second element is a simple counter which tracks how many readers are
// currently examining the map. If this counter is greater than zero, any
// thread writes are blocked until the counter reaches zero. Once this
// occurs, the pending write thread is woken back up, and performs its task
// as usual.
//

#include "pch.h"

#include "Utility/Threading/Threads.h"
#include "Utility/Threading/ThreadExceptions.h"

#include "Utility/Strings.h"

#include "User Interface/Output.h"


using namespace Threads;


//-------------------------------------------------------------------------------
// Internal data
//-------------------------------------------------------------------------------

// Externally visible constants
const wchar_t* Threads::ConsoleMutexName = L"@console-mutex";
const wchar_t* Threads::MarshallingMutexName = L"@marshalling-mutex";

// Internal tracking
namespace
{
	const wchar_t* ThreadManagerMutexName = L"@thread-manager-mutex";
	HANDLE ThreadManagerMutex;
	HANDLE ConsoleMutex;
	HANDLE ThreadStartStopGuard;
	HANDLE ThreadAccessCounterIsZero;
	unsigned ThreadAccessCounter;
	DWORD TLSIndex;

	std::map<std::wstring, ThreadInfo*> ThreadInfoTable;

	// Internal helpers
	void CleanupThisThread();
	void WaitForThreadsToFinish();
	void ClearThreadTracking();
}


//-------------------------------------------------------------------------------
// Automatic mutex acquire/release wrapper
//-------------------------------------------------------------------------------

//
// Acquire the desired mutex, blocking until ownership is gained
// Note that the mutex resource must be created by the OS prior
// to using this class.
//
AutoMutex::AutoMutex(const std::wstring& name)
{
	MutexHandle = ::OpenMutex(SYNCHRONIZE, false, name.c_str());
}

//
// Release the attached mutex
//
AutoMutex::~AutoMutex()
{
	::ReleaseMutex(MutexHandle);
}


//-------------------------------------------------------------------------------
// Synchronization counter
//-------------------------------------------------------------------------------

//
// Construct the counter wrapper and increment the counter
//
SyncCounter::SyncCounter(unsigned* pcounter, HANDLE tripevent)
	: PointerToCounter(pcounter), TripEvent(tripevent)
{
	::ResetEvent(tripevent);

	while(true)
	{
		unsigned oldval = *pcounter;
		bool success = CompareAndSwap(pcounter, oldval, oldval + 1);
		if(success)
			return;
	}
}

//
// Destruct the counter wrapper and decrement the counter
//
SyncCounter::~SyncCounter()
{
	while(true)
	{
		unsigned oldval = *PointerToCounter;
		bool success = CompareAndSwap(PointerToCounter, oldval, oldval - 1);
		if(success)
		{
			if(oldval - 1 == 0)
				::SetEvent(TripEvent);

			return;
		}
	}
}


//-------------------------------------------------------------------------------
// Thread functionality
//-------------------------------------------------------------------------------

//
// Initialize the threading management logic 
//
void Threads::Init()
{
	ThreadAccessCounter = 0;

	ThreadManagerMutex = ::CreateMutex(NULL, false, ThreadManagerMutexName);
	ConsoleMutex = ::CreateMutex(NULL, false, ConsoleMutexName);

	TLSIndex = ::TlsAlloc();
	if(TLSIndex == TLS_OUT_OF_INDEXES)
		throw ThreadException("Failed to allocate thread-local storage");

	ThreadAccessCounterIsZero = ::CreateEvent(NULL, true, true, NULL);
	ThreadStartStopGuard = ::CreateEvent(NULL, true, true, NULL);

	std::auto_ptr<ThreadInfo> threadinfo(new ThreadInfo);
	threadinfo->CodeBlock = NULL;
	threadinfo->HandleToSelf = ::GetCurrentThreadId();
	threadinfo->TaskOrigin = 0;
	threadinfo->MessageEvent = ::CreateEvent(NULL, false, false, NULL);
	threadinfo->LocalHeapHandle = ::HeapCreate(HEAP_NO_SERIALIZE, 0, 0);

	::TlsSetValue(TLSIndex, threadinfo.get());

	// This must be set AFTER the TLS is set up, because the mailbox
	// code will attempt to use the general use memory pool.
	threadinfo->Mailbox = new LocklessMailbox<MessageInfo>(NULL);

	ThreadInfoTable.insert(std::make_pair(L"@main-thread", threadinfo.release()));
}


//
// Shutdown the thread management system.
//
void Threads::Shutdown()
{
	WaitForThreadsToFinish();

	CleanupThisThread();
	ClearThreadTracking();
	::CloseHandle(ThreadManagerMutex);
	::CloseHandle(ConsoleMutex);
	::CloseHandle(ThreadStartStopGuard);
	::CloseHandle(ThreadAccessCounterIsZero);
	::TlsFree(TLSIndex);
}


//
// Create a new thread, executing the specified function
//
void Threads::Create(const std::wstring& name, ThreadFuncPtr func, VM::Block* codeblock)
{
	AutoMutex mutex(ThreadManagerMutexName);
	::WaitForSingleObject(ThreadAccessCounterIsZero, INFINITE);

	struct safety
	{
		safety()		{ ::ResetEvent(ThreadStartStopGuard); }
		~safety()		{ ::SetEvent(ThreadStartStopGuard); }
	} safetywrapper;

	std::map<std::wstring, ThreadInfo*>::const_iterator iter = ThreadInfoTable.find(name);
	if(iter != ThreadInfoTable.end())
		throw ThreadException("Cannot fork a task with this name - name is already in use!");

	std::auto_ptr<ThreadInfo> info(new ThreadInfo);

	info->CodeBlock = codeblock;
	info->MessageEvent = ::CreateEvent(NULL, false, false, NULL);
	info->TaskOrigin = reinterpret_cast<ThreadInfo*>(::TlsGetValue(TLSIndex))->HandleToSelf;
	info->BoundFuture = NULL;

	ThreadInfoTable[name] = info.get();

	HANDLE newthread = ::CreateThread(NULL, 0, func, info.get(), CREATE_SUSPENDED, &info->HandleToSelf);

	std::auto_ptr<LocklessMailbox<MessageInfo> > mailbox(new LocklessMailbox<MessageInfo>(newthread));
	info->Mailbox = mailbox.get();

	info.release();
	mailbox.release();
	::ResumeThread(newthread);
}

//
// Fork a thread specifically for computing the value of a future.
//
// In general this overload should not be called directly; instead,
// it should be accessed indirectly via the futures interface.
//
void Threads::Create(const std::wstring& name, ThreadFuncPtr func, VM::Future* boundfuture, VM::Operation* op)
{
	AutoMutex mutex(ThreadManagerMutexName);
	::WaitForSingleObject(ThreadAccessCounterIsZero, INFINITE);

	struct safety
	{
		safety()		{ ::ResetEvent(ThreadStartStopGuard); }
		~safety()		{ ::SetEvent(ThreadStartStopGuard); }
	} safetywrapper;

	std::map<std::wstring, ThreadInfo*>::const_iterator iter = ThreadInfoTable.find(name);
	if(iter != ThreadInfoTable.end())
		throw ThreadException("Cannot fork a task with this name - name is already in use!");

	std::auto_ptr<ThreadInfo> info(new ThreadInfo);

	info->OpPointer = op;
	info->MessageEvent = ::CreateEvent(NULL, false, false, NULL);
	info->TaskOrigin = reinterpret_cast<ThreadInfo*>(::TlsGetValue(TLSIndex))->HandleToSelf;
	info->BoundFuture = boundfuture;

	ThreadInfoTable[name] = info.get();

	HANDLE newthread = ::CreateThread(NULL, 0, func, info.get(), CREATE_SUSPENDED, &info->HandleToSelf);

	std::auto_ptr<LocklessMailbox<MessageInfo> > mailbox(new LocklessMailbox<MessageInfo>(newthread));
	info->Mailbox = mailbox.get();

	info.release();
	mailbox.release();
	::ResumeThread(newthread);
}

//
// Initialize a thread environment.
// All forked threads MUST call this function before executing
//
void Threads::Enter(void* info)
{
	::TlsSetValue(TLSIndex, info);
	static_cast<ThreadInfo*>(info)->LocalHeapHandle = ::HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
}

//
// Clean up tracking resources reserved for the current thread.
// All forked threads MUST call this function before returning
//
void Threads::Exit()
{
	AutoMutex mutex(ThreadManagerMutexName);

	struct safety
	{
		safety()		{ ::ResetEvent(ThreadStartStopGuard); }
		~safety()		{ ::SetEvent(ThreadStartStopGuard); }
	} safetywrapper;

	CleanupThisThread();
}


namespace
{

	//
	// Free resources used to track this thread's information
	//
	void CleanupThisThread()
	{
		for(std::map<std::wstring, ThreadInfo*>::iterator iter = ThreadInfoTable.begin(); iter != ThreadInfoTable.end(); )
		{
			if(iter->second->HandleToSelf == ::GetCurrentThreadId())
			{
				delete iter->second->Mailbox;
				::HeapDestroy(iter->second->LocalHeapHandle);
				::CloseHandle(iter->second->MessageEvent);
				delete iter->second;
				iter = ThreadInfoTable.erase(iter);
			}
			else
				++iter;
		}
	}

}


//
// Send a message to another thread
//
void Threads::SendEvent(const std::wstring& threadname, const std::wstring& eventname, const std::list<VM::EpochVariableTypeID>& payloadtypes, HeapStorage* storageblock)
{
	SyncCounter sync(&ThreadAccessCounter, ThreadAccessCounterIsZero);
	::WaitForSingleObject(ThreadStartStopGuard, INFINITE);

	std::auto_ptr<HeapStorage> storageblockwrapper(storageblock);

	std::auto_ptr<MessageInfo> msg(new MessageInfo);
	msg->MessageName = eventname;
	msg->PayloadTypes = payloadtypes;
	msg->StorageBlock = storageblock;
	msg->Origin = ::GetCurrentThreadId();

	std::map<std::wstring, ThreadInfo*>::const_iterator iter = ThreadInfoTable.find(threadname);
	if(iter == ThreadInfoTable.end())
	{
		UI::OutputStream output;
		output << UI::lightred;
		output << L"WARNING - failed to send message \"" << eventname << L"\" to task \"" << threadname;
		output << L"\"\nHas the task already exited?" << std::endl;
		output << UI::resetcolor;
		return;
	}

	iter->second->Mailbox->AddMessage(msg.release());
	::SetEvent(iter->second->MessageEvent);
	storageblockwrapper.release();
}


//
// Suspend the thread until a new message arrives
//
MessageInfo* Threads::WaitForEvent()
{
	LocklessMailbox<MessageInfo>* mailbox;

	{
		SyncCounter sync(&ThreadAccessCounter, ThreadAccessCounterIsZero);
		::WaitForSingleObject(ThreadStartStopGuard, INFINITE);

		mailbox = reinterpret_cast<ThreadInfo*>(::TlsGetValue(TLSIndex))->Mailbox;

		MessageInfo* mail = mailbox->GetMessage();
		if(mail)
			return mail;
	}

	::WaitForSingleObject(reinterpret_cast<ThreadInfo*>(::TlsGetValue(TLSIndex))->MessageEvent, INFINITE);
	return mailbox->GetMessage();
}

namespace
{

	//
	// Clean up all thread tracking
	//
	void ClearThreadTracking()
	{
		for(std::map<std::wstring, ThreadInfo*>::iterator iter = ThreadInfoTable.begin(); iter != ThreadInfoTable.end(); ++iter)
		{
			::HeapDestroy(iter->second->LocalHeapHandle);
			::CloseHandle(iter->second->MessageEvent);
			delete iter->second->Mailbox;
			delete iter->second;
		}

		ThreadInfoTable.clear();
	}

}


//
// Look up a thread's name given its ID number.
// This has to do a little bit of magic to avoid searching the
// name map without introducing synchronization problems.
// See the header comments of this file for details.
//
std::wstring Threads::GetThreadNameGivenID(unsigned id)
{
	SyncCounter sync(&ThreadAccessCounter, ThreadAccessCounterIsZero);
	::WaitForSingleObject(ThreadStartStopGuard, INFINITE);

	for(std::map<std::wstring, ThreadInfo*>::iterator iter = ThreadInfoTable.begin(); iter != ThreadInfoTable.end(); ++iter)
	{
		if(iter->second->HandleToSelf == id)
			return iter->first;
	}

	throw ThreadException("Could not locate any task with the given ID; has it already finished execution?");
}


//
// Sit around until all threads exit. Mainly useful
// for when the VM shuts down and we need to let
// things terminate gracefully.
//
void Threads::WaitForThreadsToFinish()
{
	while(ThreadInfoTable.size() > 1)		// The main thread will remain registered, so we count down to 1 instead of 0
	{
		::Sleep(100);
	}
}

//
// Retrieve the thread's information block from thread-local storage
//
const ThreadInfo& Threads::GetInfoForThisThread()
{
	return *reinterpret_cast<ThreadInfo*>(::TlsGetValue(TLSIndex));
}
