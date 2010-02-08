//
// The Epoch Language Project
// Shared Library Code
//
// Implementation of a lock-free mailbox structure for message passing
//

#pragma once

// Dependencies
#include "Utility/Threading/Lockless.h"
#include "Utility/Threading/Threads.h"

#include "Utility/Types/IDTypes.h"

#include "Utility/Memory/ThreadLocalAllocator.h"

#include "Configuration/RuntimeOptions.h"



//
// This class encapsulates a lockless mailbox algorithm for asynchronous message passing.
// Each thread is granted a mailbox when it is started up; this mailbox is used for all
// messages passed into that thread. Messages are dequeued in FIFO order.
//
// IMPORTANT: this algorithm is only safe for a many producer/single consumer scenario.
//            Critical assumptions made by the code are only valid if a single consumer
//            thread is used. However, any number of producer threads is permitted.
//
// The algorithm uses three distinct stacks: a read stack, a write stack, and a cache.
// Producers push items onto the write stack using a simple and well-known CAS method.
// When the consumer thread arrives to retrieve a message, it first checks the cache
// for any waiting messages. If the cache is empty, the consumer thread swaps the read
// and write stacks. At this point, the read stack is controlled entirely by the consumer
// thread, so we don't need to worry about contention for elements in the read stack.
// The entire read stack is then traversed and its contents pushed into the cache.
// Note that we achieve FIFO semantics only by using this extra cache stack.
//
// For additional performance, we use customized memory management internally to avoid
// expensive serialized heap allocations/frees. We cannot directly use new/delete in
// the implementation, because these incur serialized heap accesses, which defeats the
// purpose of the lock-free mailbox. Instead, we use a two-pronged approach to memory
// management for the mailbox. Mail messages are kept in a pre-allocated buffer, and
// memory is supplied by this buffer using a simple lock-free LIFO stack of free slots.
// We also use a special allocator class for the std::stack container, which uses a
// thread-local heap provided by the OS. Since the thread-local heap is never accessed
// by an outside thread, we can avoid the cost of serialized heap accesses to that
// particular memory. The combination of these two approaches allows for maximum speed
// in the mailbox container, while retaining lock-free semantics and thread safety.
//
template <class PayloadType>
class LocklessMailbox
{
// Construction and destruction
public:
	//
	// Construct and initialize the read and write stacks
	//
	LocklessMailbox(HANDLE owner)
		: Owner(owner)
	{
		WriteHead = OriginalWriteHead = new Node;
		ReadHead = OriginalReadHead = new Node;

		unsigned buffersize = Config::NumMessageSlots;
		NodeBuffer = new Node[buffersize];

		NodeFreeList = NULL;
		for(unsigned i = 0; i < buffersize; ++i)
		{
			NodeTracker* tracker = new NodeTracker(NodeFreeList, &NodeBuffer[i]);
			FreeListHead = NodeFreeList = tracker;
			NodeBuffer[i].Tracker = tracker;
		}
	}

	//
	// Clean up the read, write, and cache stacks, freeing any
	// remaining messages from each stack.
	//
	~LocklessMailbox()
	{
		for(MessageStackType::iterator iter = PendingReads.begin(); iter != PendingReads.end(); ++iter)
			delete (*iter)->Payload;

		Node* n = WriteHead;
		while(n)
		{
			delete n->Payload;
			n = n->Next;
		}

		n = ReadHead;
		while(n)
		{
			delete n->Payload;
			n = n->Next;
		}

		for(unsigned i = 0; i < Config::NumMessageSlots; ++i)
			delete NodeBuffer[i].Tracker;

		delete OriginalReadHead;
		delete OriginalWriteHead;

		delete [] NodeBuffer;
	}

// Message passing interface
public:

	//
	// Register a message from a producer thread. Any number of threads
	// may call this function.
	//
	void AddMessage(PayloadType* info)
	{
		Node* msgnode = AllocateNode(info);
		while(true)
		{
			msgnode->Next = WriteHead;
			bool success = CompareAndSwap(&WriteHead, msgnode->Next, msgnode);
			if(success)
				return;
		}
	}

	//
	// Retrieve a message from the mailbox.
	// IMPORTANT: only ONE consumer thread (per mailbox) should call this function
	//
	PayloadType* GetMessage()
	{
		if(!PendingReads.empty())
			return PopPendingRead();

		if(ReadHead->Next == NULL)
			SwapReadAndWrite();

		Node* n = ReadHead;
		while(ReadHead->Next)
		{
			PendingReads.push_back(n);
			n = n->Next;
			ReadHead = n;
		}

		if(PendingReads.empty())
			return NULL;

		return PopPendingRead();
	}

// Internal helpers
private:

	//
	// Pop a pending read from the cache stack
	//
	PayloadType* PopPendingRead()
	{
		Node* readnode = PendingReads.back();
		PayloadType* payload = readnode->Payload;
		FreeNode(readnode);
		PendingReads.pop_back();
		return payload;
	}

	//
	// Internal helper for swapping the read/write stacks
	// See class comment for details
	//
	void SwapReadAndWrite()
	{
		while(true)
		{
			Node* readhead = ReadHead;
			Node* oldwrite = WriteHead;
			bool success = CompareAndSwap(&WriteHead, oldwrite, readhead);
			if(success)
			{
				ReadHead = oldwrite;
				return;
			}
		}
	}

// Internal memory management
private:
	struct NodeTracker;

	struct Node
	{
		Node() : Next(NULL), Payload(NULL) { }
		explicit Node(PayloadType* p) : Next(NULL), Payload(p) { }

		Node* Next;
		PayloadType* Payload;
		NodeTracker* Tracker;
	};

	struct NodeTracker
	{
		NodeTracker() : Next(NULL), AttachedNode(NULL) { }
		explicit NodeTracker(Node* attached) : Next(NULL), AttachedNode(attached) { }
		NodeTracker(NodeTracker* next, Node* attached) : Next(next), AttachedNode(attached) { }

		NodeTracker* Next;
		Node* AttachedNode;
	};

	//
	// Allocate space for a message node, using the local reserved pool
	//
	// We trade off a limited amount of message space for the ability
	// to allocate and free node storage without locking on the heap.
	// Memory is managed with a simple free list that stores pointers
	// into the pre-allocated node buffer. The list is implemented as
	// a lock-free FILO stack; this allows multiple producers to send
	// messages without blocking on the allocation routines.
	//
	Node* AllocateNode(PayloadType* info)
	{
		NodeTracker* rettracker;

		if(!FreeListHead->Next)
		{
			::TerminateThread(Owner, 0);
			throw Exception("Too many messages backlogged; make sure task is accepting the sent messages!");
		}

		// Pull node out of the free list
		NodeTracker* newhead;
		do
		{
			rettracker = FreeListHead;
			newhead = rettracker->Next;
		} while (!CompareAndSwap(&FreeListHead, rettracker, newhead));

		rettracker->AttachedNode->Payload = info;
		return rettracker->AttachedNode;
	}

	//
	// Return a node to the free list
	//
	void FreeNode(Node* node)
	{
		node->Payload = NULL;

		while(true)
		{
			node->Tracker->Next = FreeListHead;
			if(CompareAndSwap(&FreeListHead, node->Tracker->Next, node->Tracker))
				break;
		}
	}


// Internal tracking
private:
	Node* WriteHead, *OriginalWriteHead;
	Node* ReadHead, *OriginalReadHead;

	Node* NodeBuffer;

	NodeTracker* NodeFreeList;
	NodeTracker* FreeListHead;

	typedef std::deque<Node*, ThreadLocalAllocator<Node*> > MessageStackType;
	MessageStackType PendingReads;

	HANDLE Owner;
};


