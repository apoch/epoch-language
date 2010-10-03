//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Memory management subsystems. These classes handle the internal
// memory allocations and services for the virtual machine.
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Threading/Synchronization.h"

#include <vector>


//
// Platform-independent wrapper for managing heaps.
//
class HeapManager
{
// Construction and destruction
public:
	HeapManager();
	~HeapManager();

// System parameter retrieval
public:
	HANDLE GetHeap() const
	{ return HeapHandle; }

	unsigned GetPageSize() const
	{ return PageSize; }

// Global access to the heap wrapper
public:
	static HeapManager& GetGlobalHeapManager()
	{
		Threads::CriticalSection::Auto lock(CritSec);

		if(!GlobalHeapManager)
			GlobalHeapManager = new HeapManager;
		return *GlobalHeapManager;
	}

// Internal storage
private:
	HANDLE HeapHandle;
	unsigned PageSize;

// Shared internal storage
private:
	static HeapManager* GlobalHeapManager;
	static Threads::CriticalSection CritSec;
};

HeapManager& GetSingleGlobalHeapManager();



//
// Base interface for all memory allocators.
//
class MemoryAllocator
{
// Destruction
public:
	virtual ~MemoryAllocator()
	{ }

// Memory operations
public:
	virtual void AllocateBlock(size_t numbytes) = 0;
	virtual void* CommitMemory(size_t numbytes) = 0;
	virtual void FreeMemory(void* address) = 0;
};


//
// This memory allocator is the simplest and has the least overhead, both in
// terms of allocation/deallocation complexity, and in terms of extra memory
// padding. All memory is packed as tightly as possible on the heap without
// regards for alignment.
//
class UnalignedMemoryAllocator : public MemoryAllocator
{
// Construction and destruction
public:
	UnalignedMemoryAllocator(size_t initialblocksize);
	virtual ~UnalignedMemoryAllocator();

// Memory operations
public:
	virtual void AllocateBlock(size_t numbytes);
	virtual void* CommitMemory(size_t numbytes);
	virtual void FreeMemory(void* address);

protected:
	//
	// Helper structure for recording allocations
	//
	struct AllocBlock
	{
		void* Storage;
		size_t NumBytes;
		size_t FreeOffset;

		//
		// Initialize the block tracking structure
		//
		AllocBlock(void* storage, size_t numbytes)
			: Storage(storage), NumBytes(numbytes), FreeOffset(0)
		{ }

		//
		// Commit a chunk of memory out of the allocated space
		// May throw an exception if too much space is committed
		//
		void* Commit(size_t numbytes)
		{
			// WARNING - this code makes a platform-dependent assumption that char is 1 byte
			const size_t PLATFORM_CHAR_NUMBYTES = 1;

			void* ptr = reinterpret_cast<Byte*>(Storage) + (FreeOffset / PLATFORM_CHAR_NUMBYTES);
			FreeOffset += numbytes;

			if(FreeOffset > NumBytes)
				throw MemoryException("Overcommitted an unaligned allocation block!");

			return ptr;
		}
	};

// Internal storage
protected:
	std::vector<AllocBlock> AllocBlocks;
};

