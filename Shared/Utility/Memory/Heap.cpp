//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Definition of wrapper objects for managing heap storage
//

#include "pch.h"
#include "Utility/Memory/Heap.h"



//
// Construct and initialize the heap storage wrapper
//
HeapStorage::HeapStorage()
	: AllocatedSpace(NULL)
{
}

//
// Destruct and clean up the heap storage wrapper
//
HeapStorage::~HeapStorage()
{
	delete [] AllocatedSpace;
}

//
// Allocate heap space for the wrapper
// WARNING: any previous space is deleted!
//
void HeapStorage::Allocate(size_t numbytes)
{
	delete [] AllocatedSpace;
	AllocatedSpace = new Byte[numbytes];
}

