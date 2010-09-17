//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Implementation of memory management subsystems.
//

#include "pch.h"
#include "Utility/Memory/MemoryManager.h"

HeapManager* HeapManager::GlobalHeapManager = NULL;

//
// Construct a heap manager. This should take care of any platform
// specific operations to ensure that subsequent allocation requests
// can be serviced.
//
HeapManager::HeapManager()
{
	SYSTEM_INFO sysinfo;
	::GetSystemInfo(&sysinfo);
	PageSize = sysinfo.dwPageSize;

	HeapHandle = ::HeapCreate(0, PageSize, 0);
	if(!HeapHandle)
		throw std::exception("Failed to allocate system heap!");
}

//
// Clean up a heap; this should take care of any platform specific
// operations for releasing the heap.
//
HeapManager::~HeapManager()
{
	::HeapDestroy(HeapHandle);
}


//
// Construct a wrapper for managing unaligned memory blocks.
//
UnalignedMemoryAllocator::UnalignedMemoryAllocator(size_t initialblocksize)
{
	AllocateBlock(initialblocksize);
}

//
// Destruct an unaligned memory manager and free all associated blocks.
//
UnalignedMemoryAllocator::~UnalignedMemoryAllocator()
{
	for(std::vector<AllocBlock>::iterator iter = AllocBlocks.begin(); iter != AllocBlocks.end(); ++iter)
		::HeapFree(GetSingleGlobalHeapManager().GetHeap(), 0, iter->Storage);
}

//
// Allocate a block on the heap. This should generally only be used as a kind of
// reservation mechanism when it is known that memory will be needed in the future;
// for actual allocations, use CommitMemory. Note that the minimum block size is
// one system-specific page.
//
void UnalignedMemoryAllocator::AllocateBlock(size_t numbytes)
{
	size_t numpages = (numbytes / GetSingleGlobalHeapManager().GetPageSize()) + 1;

	void* memory = ::HeapAlloc(GetSingleGlobalHeapManager().GetHeap(), 0, numpages * GetSingleGlobalHeapManager().GetPageSize());
	if(!memory)
		throw std::exception("Failed to allocate heap memory!");

	AllocBlocks.push_back(AllocBlock(memory, numbytes));
}

//
// Commit a block of memory. This will use existing memory chunks if possible,
// or allocate new pages as necessary.
//
void* UnalignedMemoryAllocator::CommitMemory(size_t numbytes)
{
	for(std::vector<AllocBlock>::iterator iter = AllocBlocks.begin(); iter != AllocBlocks.end(); ++iter)
	{
		AllocBlock& block = *iter;
		if(block.FreeOffset + numbytes <= block.NumBytes)
			return block.Commit(numbytes);
	}

	AllocateBlock(numbytes);
	return AllocBlocks.rbegin()->Commit(numbytes);
}


//
// Release a block of memory. This marks a region of memory as freed; memory
// which is freed can be consolidated during the garbage collection phase,
// and then be reused for new allocations.
//
void UnalignedMemoryAllocator::FreeMemory(void* address)
{
	// TODO - implement memory release mechanism and garbage collection
}

