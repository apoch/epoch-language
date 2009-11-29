//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Declaration of wrapper objects for managing heap storage
//

#pragma once


class HeapStorage
{
// Construction and destruction
public:
	HeapStorage();
	~HeapStorage();

// Memory management interface
public:
	void Allocate(size_t numbytes);

	void* GetStartOfStorage() const
	{ return AllocatedSpace; }

// Internal tracking
private:
	Byte* AllocatedSpace;
};

