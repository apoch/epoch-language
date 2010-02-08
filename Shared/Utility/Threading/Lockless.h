//
// The Epoch Language Project
// Shared Library Code
//
// Basic building blocks for creating lock-free algorithms
//

#pragma once

//
// Helper function for atomic compare-and-swap
// Returns true on success
//
template <class DataType>
bool CompareAndSwap(DataType* field, DataType oldvalue, DataType newvalue)
{
	DataType retval;

	_asm
	{
		mfence
		mov eax, oldvalue
		mov ecx, newvalue
		mov edx, dword ptr [field]

		lock cmpxchg dword ptr[edx], ecx
		mov retval, eax
		mfence
	}

	return (retval == oldvalue);
}

