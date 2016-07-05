#include "pch.h"



#include "GCCompilation.h"



using namespace llvm;

namespace
{

	struct GCLiveRootInfo
	{
		int32_t StackOffset;
	};

	struct GCRootData
	{
		uint64_t LabelOffset;
		const Function* OriginalFunction;
		uint64_t StackFrameSize;
		uint32_t RootsIndex;
		uint32_t RootsCount;
	};


	struct GCData
	{
		~GCData()
		{
			for(auto p : RootData)
				delete p;
		}

		std::vector<GCRootData*> RootData;
		std::vector<GCLiveRootInfo> LiveRootCache;
	};


	GCData CompilationGCData;


	class LLVM_LIBRARY_VISIBILITY EpochGCStrategy : public GCStrategy
	{
	public:
		EpochGCStrategy()
		{
			UsesMetadata = true;
			NeededSafePoints = 1 << GC::PostCall;
		}


		void RegisterSafePointData(GCFunctionInfo& func)
		{
			uint32_t rootindex = CompilationGCData.LiveRootCache.size();
			uint32_t rootcount = 0;

			for(auto liveiter = func.live_begin(func.begin()); liveiter != func.live_end(func.begin()); ++liveiter)
			{
				GCLiveRootInfo root;
				root.StackOffset = liveiter->StackOffset;

				CompilationGCData.LiveRootCache.push_back(root);
				++rootcount;
			}


			for(auto safepointiter = func.begin(); safepointiter != func.end(); ++safepointiter)
			{
				auto safepoint = *safepointiter;

				GCRootData* rootdata = new GCRootData;
				rootdata->OriginalFunction = &func.getFunction();
				rootdata->StackFrameSize = func.getFrameSize();
				rootdata->LabelOffset = safepoint.Label->getOffset();
				rootdata->RootsIndex = rootindex;
				rootdata->RootsCount = rootcount;

				CompilationGCData.RootData.push_back(rootdata);
			}
		}
	};


	class LLVM_LIBRARY_VISIBILITY EpochGCPrinter : public GCMetadataPrinter
	{
	public:
		virtual void finishAssembly(Module& module, GCModuleInfo& info, AsmPrinter& printer) override
		{
			for(auto iter = info.funcinfo_begin(); iter != info.funcinfo_end(); ++iter)
			{
				GCFunctionInfo& func = **iter;

				if(func.getStrategy().getName() != "EpochGC")
					continue;

				EpochGCStrategy& strategy = static_cast<EpochGCStrategy&>(func.getStrategy());
				strategy.RegisterSafePointData(func);
			}
		}
	};



	GCRegistry::Add<EpochGCStrategy>				RegisterStrategy("EpochGC", "Epoch Language Runtime GC");
	GCMetadataPrinterRegistry::Add<EpochGCPrinter>	RegisterPrinter("EpochGC", "Epoch Language Runtime GC");




	template<typename T>
	void AppendToBuffer(std::vector<char>* buffer, const T& data)
	{
		const char* pdata = reinterpret_cast<const char*>(&data);
		for(size_t i = 0; i < sizeof(T); ++i)
		{
			buffer->push_back(*pdata);
			++pdata;
		}
	}



}




void GCCompilation::PrepareGCData(llvm::ExecutionEngine& ee, std::vector<char>* sectiondata)
{
	sectiondata->clear();

	AppendToBuffer(sectiondata, static_cast<uint32_t>(CompilationGCData.RootData.size()));

	for(auto rd : CompilationGCData.RootData)
	{
		void* funcraw = ee.getPointerToFunction(const_cast<Function*>(rd->OriginalFunction));
		const char* funcentryaddr = reinterpret_cast<const char*>(funcraw);
		const char* safepointip = funcentryaddr + rd->LabelOffset - 0x400000;

		AppendToBuffer(sectiondata, static_cast<uint32_t>(reinterpret_cast<uint64_t>(safepointip)));
		AppendToBuffer(sectiondata, static_cast<uint32_t>(rd->StackFrameSize));
		AppendToBuffer(sectiondata, static_cast<uint32_t>(rd->RootsIndex));
		AppendToBuffer(sectiondata, static_cast<uint32_t>(rd->RootsCount));
	}

	for(const auto& root : CompilationGCData.LiveRootCache)
	{
		AppendToBuffer(sectiondata, root);
	}
}




