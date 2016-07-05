#include "stdafx.h"
#include "GC.h"

#include "StringPool.h"


namespace
{

	struct GCFunctionData
	{
		uint32_t BeginOffsetIP;
		uint32_t EndOffsetIP;
		uint32_t StartOfGCData;
	};


	struct GCImageTable
	{
		unsigned NumEntries;
		const GCFunctionData* Entries;
		const char* StartOfGCData;
	};


	GCImageTable GCTable;




	void TraceStackRoots(uint64_t instructionptr)
	{
		uint64_t baseaddr = reinterpret_cast<uint64_t>(::GetModuleHandle(NULL));
		uint64_t instructionoffset = instructionptr - baseaddr;

		const GCFunctionData* functiondata = GCTable.Entries;

		for(unsigned i = 0; i < GCTable.NumEntries; ++i, ++functiondata)
		{
			if(instructionoffset < functiondata->BeginOffsetIP)
				continue;

			if(instructionoffset >= functiondata->EndOffsetIP)
				continue;

			const char* gcdataptr = GCTable.StartOfGCData + functiondata->StartOfGCData;

			// TODO - stash LLVM stack map at gcdataptr, traverse it here for roots
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

			TraceStackRoots(reinterpret_cast<uint64_t>(p));
		}
	}


}



void GC::Init(uint32_t gcsectionoffset)
{
	const char* baseofprocess = reinterpret_cast<const char*>(::GetModuleHandle(NULL));
	const char* gcsection = baseofprocess + gcsectionoffset;

	GCTable.NumEntries = *reinterpret_cast<const unsigned*>(gcsection);
	GCTable.Entries = reinterpret_cast<const GCFunctionData*>(gcsection + sizeof(unsigned));
	GCTable.StartOfGCData = reinterpret_cast<const char*>(GCTable.Entries) + GCTable.NumEntries * sizeof(GCFunctionData);
}



void GC::CollectStrings(ThreadStringPool * pool, void* retaddr)
{
	StackCrawl();
	pool->FreeUnusedEntries();
}

