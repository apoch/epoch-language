//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Implementation of the basic push-down, downward-growing stack
// used during execution of code within the virtual machine.
//
// Note that we do not provide methods for directly pushing or
// popping values onto and off the stack. Instead, we push/pop
// the requisite number of bytes, and callers are responsible
// for retrieving newly allocated stack addresses. This safely
// and cleanly decouples the stack from any client code, and
// allows any form of data to be stored without code overhead.
//


#include "pch.h"

#include "Utility/Memory/Stack.h"
#include "Utility/Memory/MemoryManager.h"

#include "Configuration/RuntimeOptions.h"


//
// WARNING - this code makes a platform-dependent assumption that char is 1 byte
//

//
// Construct a stack and allocate the default amount of space.
//
StackSpace::StackSpace()
{
	StackAllocation = ::HeapAlloc(TheGlobalHeap.GetHeap(), 0, Config::StackSize);
	CurrentStackPointer = EndOfStackAllocation = reinterpret_cast<Byte*>(StackAllocation) + Config::StackSize;
#ifdef _DEBUG
	memset(StackAllocation, 0xee, Config::StackSize);
#endif
}

//
// Construct a stack of the given size.
//
StackSpace::StackSpace(size_t numbytes)
{
	StackAllocation = ::HeapAlloc(TheGlobalHeap.GetHeap(), 0, numbytes);
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
	::HeapFree(TheGlobalHeap.GetHeap(), 0, StackAllocation);
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
#ifdef _DEBUG
		// For debug purposes, calculate how much excess space was popped
		ptrdiff_t diff = reinterpret_cast<Byte*>(CurrentStackPointer) - reinterpret_cast<Byte*>(EndOfStackAllocation);
#endif

		CurrentStackPointer = EndOfStackAllocation;
		throw MemoryException("Popped too much off the stack");
	}
}


//
// END platform-dependent assumptions
//