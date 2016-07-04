#include "stdafx.h"
#include "GC.h"

#include "StringPool.h"


namespace
{


	struct GCImageTable
	{
		unsigned NumEntries;
	};


	const GCImageTable* GCTable = nullptr;




	void TraceStackRoots(void* instructionptr)
	{
		for(unsigned i = 0; i < GCTable->NumEntries; ++i)
		{
			// TODO - if this GCTable entry matches IP, look up LLVM stack data and traverse it
		}
	}

	void StackCrawl()
	{
		PVOID trace[64] = {};

		CaptureStackBackTrace(3, 61, trace, NULL);

		for(PVOID p : trace)
		{
			if(!p)
				break;

			TraceStackRoots(p);
		}
	}


}



void GC::Init(uint32_t gcsectionoffset)
{
	const char* baseofprocess = reinterpret_cast<const char*>(::GetModuleHandle(NULL));
	const char* gcsection = baseofprocess + gcsectionoffset;

	GCTable = reinterpret_cast<const GCImageTable*>(gcsection);
}



void GC::CollectStrings(ThreadStringPool * pool, void* retaddr)
{
	StackCrawl();
	pool->FreeUnusedEntries();
}

