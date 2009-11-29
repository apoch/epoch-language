//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Definition of the basic push-down, downward-growing stack
// used during execution of code within the virtual machine.
//

#pragma once


//
// Basic implementation of a downward-growing stack
//
class StackSpace
{
// Construction and destruction
public:
	StackSpace();
	StackSpace(size_t numbytes);

	~StackSpace();

// Stack manipulation
public:
	void Push(size_t numbytes);
	void Pop(size_t numbytes);

// Stack address retrieval
public:
	void* GetCurrentTopOfStack() const
	{ return CurrentStackPointer; }

	// WARNING - this code makes a platform-dependent assumption that char is 1 byte
	void* GetOffsetIntoStack(size_t numbytes) const
	{ return reinterpret_cast<Byte*>(CurrentStackPointer) + numbytes; }

// Statistics
public:
	size_t GetAllocatedStack() const
	{ return reinterpret_cast<Byte*>(EndOfStackAllocation) - reinterpret_cast<Byte*>(CurrentStackPointer); }
	// END platform-dependent assumptions

// Internal tracking
private:
	void* StackAllocation;
	void* EndOfStackAllocation;
	void* CurrentStackPointer;
};

