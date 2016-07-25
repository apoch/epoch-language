#include "stdafx.h"
#include "GC.h"

#include "StringPool.h"


#include <DbgHelp.h>


namespace
{

	struct GCRootData
	{
		int StackFrameOffset;
		uint32_t TypeID;
	};

	struct GCSafePointData
	{
		uint32_t ReturnIP;
		uint32_t StackFrameSize;
		uint32_t RootDataIndex;
		uint32_t RootDataCount;
	};

	struct GCImageTable
	{
		unsigned NumEntries;
		const GCSafePointData* Entries;
		const GCRootData* Roots;
	};


	GCImageTable GCTable;


	void WalkStackRoots(uint64_t stackptr, uint32_t framesize, uint32_t rootindex, uint32_t rootcount, ThreadStringPool* stringpool)
	{
		for(uint32_t i = 0; i < rootcount; ++i)
		{
			const auto& root = GCTable.Roots[i + rootindex];

			int offset = root.StackFrameOffset;
			uint32_t type = root.TypeID;

			// TODO - stash structure data, walk heap for references

			if(type == 0x02000000)
			{
				// Temp Proof of Concept hack
				const char** foo = (const char**)((const char*)(stackptr + offset));
				stringpool->MarkInUse(*foo);
			}
		}
	}

	void TraceStackRoots(uint64_t instructionptr, uint64_t stackptr, ThreadStringPool* stringpool)
	{
		uint64_t baseaddr = reinterpret_cast<uint64_t>(::GetModuleHandle(NULL));
		uint64_t instructionoffset = instructionptr - baseaddr;

		const GCSafePointData* safepointdata = GCTable.Entries;

		for(unsigned i = 0; i < GCTable.NumEntries; ++i, ++safepointdata)
		{
			if(instructionoffset != safepointdata->ReturnIP)
				continue;

			WalkStackRoots(stackptr, safepointdata->StackFrameSize, safepointdata->RootDataIndex, safepointdata->RootDataCount, stringpool);
		}
	}

	void StackCrawl(ThreadStringPool* stringpool)
	{
		CONTEXT ctx;
		memset(&ctx, 0, sizeof(ctx));
		ctx.ContextFlags = CONTEXT_FULL;
		RtlCaptureContext(&ctx);

		STACKFRAME64 frame;
		memset(&frame, 0, sizeof(frame));

		frame.AddrPC.Offset = ctx.Rip;
		frame.AddrPC.Mode = AddrModeFlat;

		frame.AddrFrame.Offset = ctx.Rbp;
		frame.AddrFrame.Mode = AddrModeFlat;

		frame.AddrStack.Offset = ctx.Rsp;
		frame.AddrStack.Mode = AddrModeFlat;

		unsigned skip = 2;
		unsigned hack = 0;

		while(::StackWalk64(IMAGE_FILE_MACHINE_AMD64, ::GetCurrentProcess(), ::GetCurrentThread(), &frame, &ctx, NULL, NULL, NULL, NULL))
		{
			if(!frame.AddrPC.Offset)
				break;

			if(skip > 0)
			{
				--skip;
				continue;
			}

			char buffer[1024] = {0};
			SYMBOL_INFO* syminfo = (SYMBOL_INFO*)(buffer);
			syminfo->SizeOfStruct = sizeof(SYMBOL_INFO);
			syminfo->MaxNameLen = 256;


			DWORD64 displ = 0;
			BOOL success = ::SymFromAddr(::GetCurrentProcess(), frame.AddrPC.Offset, &displ, syminfo);

			TraceStackRoots(frame.AddrPC.Offset, frame.AddrStack.Offset, stringpool);
		}
	}


}



void GC::Init(uint32_t gcsectionoffset)
{
	const char* baseofprocess = reinterpret_cast<const char*>(::GetModuleHandle(NULL));
	const char* gcsection = baseofprocess + gcsectionoffset;

	GCTable.NumEntries = *reinterpret_cast<const unsigned*>(gcsection);
	GCTable.Entries = reinterpret_cast<const GCSafePointData*>(gcsection + sizeof(unsigned));
	GCTable.Roots = reinterpret_cast<const GCRootData*>(reinterpret_cast<const char*>(GCTable.Entries) + GCTable.NumEntries * sizeof(GCSafePointData));


	::SymSetOptions(::SymGetOptions() | SYMOPT_DEBUG);
	::SymInitialize(::GetCurrentProcess(), NULL, TRUE);
}



void GC::CollectStrings(ThreadStringPool* pool, void* retaddr)
{
	pool->ToggleTraceBit();
	StackCrawl(pool);
	pool->FreeUnusedEntries();
}

