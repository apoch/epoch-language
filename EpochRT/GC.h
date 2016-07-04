#pragma once


class ThreadStringPool;


namespace GC
{
	void Init(uint32_t gcsectionoffset);

	void CollectStrings(ThreadStringPool * pool, void* retaddr);
}

