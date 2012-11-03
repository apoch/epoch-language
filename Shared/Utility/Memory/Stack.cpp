//
// The Epoch Language Project
// Epoch Virtual Machine
//
// Implementation of the basic push-down, downward-growing stack
// used during execution of code within the virtual machine.
//


#include "pch.h"

#include "Utility/Memory/Stack.h"
#include "Utility/Memory/MemoryManager.h"


//
// WARNING - this code makes a platform-dependent assumption that char is 1 byte
//

static const unsigned STACK_SIZE = 4 * 1024 * 1024;

//
// Construct a stack and allocate the default amount of space.
//
StackSpace::StackSpace()
{
	StackAllocation = GetSingleGlobalHeapManager().Allocate(STACK_SIZE);
	CurrentStackPointer = EndOfStackAllocation = reinterpret_cast<Byte*>(StackAllocation) + STACK_SIZE;
#ifdef _DEBUG
	memset(StackAllocation, 0xee, STACK_SIZE);
#endif
}

//
// Construct a stack of the given size.
//
StackSpace::StackSpace(size_t numbytes)
{
	StackAllocation = GetSingleGlobalHeapManager().Allocate(numbytes);
	CurrentStackPointer = EndOfStackAllocation = reinterpret_cast<Byte*>(StackAllocation) + numbytes;
#ifdef _DEBUG
	memset(StackAllocation, 0xee, numbytes);
#endif
}

//
// Clean up the stack's memory.
//
StackSpace::~StackSpace()
{
	GetSingleGlobalHeapManager().Deallocate(StackAllocation);
}

//
// Push a given number of bytes onto the stack.
//
void StackSpace::Push(size_t numbytes)
{
	CurrentStackPointer = reinterpret_cast<Byte*>(CurrentStackPointer) - numbytes;
	if(CurrentStackPointer <= StackAllocation)
	{
		CurrentStackPointer = StackAllocation;
		throw MemoryException("Out of stack space");
	}
}

//
// Pop a given number of bytes off the stack.
//
void StackSpace::Pop(size_t numbytes)
{
#ifdef _DEBUG
	memset(CurrentStackPointer, 0xcc, numbytes);
#endif
	CurrentStackPointer = reinterpret_cast<Byte*>(CurrentStackPointer) + numbytes;
	if(CurrentStackPointer > EndOfStackAllocation)
	{
		CurrentStackPointer = EndOfStackAllocation;
		throw MemoryException("Popped too much off the stack");
	}
}

//
// END platform-dependent assumptions
//