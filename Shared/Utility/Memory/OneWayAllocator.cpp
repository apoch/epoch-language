#include "pch.h"

#include "Utility/Memory/OneWayAllocator.h"
#include "Utility/Types/IntegerTypes.h"

#include <iostream>


namespace
{
	struct Block
	{
		Byte* AllocatedBuffer;
		Byte* CurrentOffset;
		size_t RemainingSize;
	};

	std::list<Block> Blocks;

	static const size_t BLOCKSIZE = 25 * 1024 * 1024;		// nMB blocks

	Integer64 TotalAllocSize = 0;
}


void* Memory::OneWayAllocate(size_t bytes)
{
	TotalAllocSize += bytes;

	if(Blocks.empty() || Blocks.back().RemainingSize < bytes)
	{
		if(bytes > BLOCKSIZE)
		{
			Byte* ret = new Byte[bytes];
			Block newblock;
			newblock.AllocatedBuffer = ret;
			newblock.CurrentOffset = newblock.AllocatedBuffer;
			newblock.RemainingSize = 0;
			Blocks.push_back(newblock);
			return ret;
		}

		Byte* ret = new Byte[BLOCKSIZE];
		Block newblock;
		newblock.AllocatedBuffer = ret;
		newblock.CurrentOffset = newblock.AllocatedBuffer + bytes;
		newblock.RemainingSize = BLOCKSIZE - bytes;
		Blocks.push_back(newblock);
		return ret;
	}
	else
	{
		Block& b = Blocks.back();
		Byte* ret = b.CurrentOffset;
		b.RemainingSize -= bytes;
		b.CurrentOffset += bytes;
		return ret;
	}
}

void Memory::OneWayRecordDealloc(size_t bytes)
{
	TotalAllocSize -= bytes;
}

void Memory::DisposeOneWayBlocks()
{
	if(TotalAllocSize != 0)
		throw std::exception("Memory leaked from one-way allocator");

	for(std::list<Block>::iterator iter = Blocks.begin(); iter != Blocks.end(); ++iter)
		delete [] iter->AllocatedBuffer;

	Blocks.clear();
}

