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
		std::vector<GCLiveRootInfo>* Roots;
	};


	struct GCData
	{
		~GCData()
		{
			for(auto p : RootData)
				delete p;

			for(auto p : LiveRootCache)
				delete p;
		}

		std::vector<GCRootData*> RootData;
		std::vector<std::vector<GCLiveRootInfo>*> LiveRootCache;
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
			std::vector<GCLiveRootInfo>* liveroots = new std::vector<GCLiveRootInfo>;
			CompilationGCData.LiveRootCache.push_back(liveroots);
				
			for(auto liveiter = func.live_begin(func.begin()); liveiter != func.live_end(func.begin()); ++liveiter)
			{
				GCLiveRootInfo root;
				root.StackOffset = liveiter->StackOffset;

				liveroots->push_back(root);
			}


			for(auto safepointiter = func.begin(); safepointiter != func.end(); ++safepointiter)
			{
				auto safepoint = *safepointiter;

				GCRootData* rootdata = new GCRootData;
				rootdata->OriginalFunction = &func.getFunction();
				rootdata->StackFrameSize = func.getFrameSize();
				rootdata->LabelOffset = safepoint.Label->getOffset();
				rootdata->Roots = liveroots;

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

}




void GCCompilation::DumpGCData(llvm::ExecutionEngine& ee)
{
	for(auto rd : CompilationGCData.RootData)
	{
		void* funcraw = ee.getPointerToFunction(const_cast<Function*>(rd->OriginalFunction));
		const char* funcentryaddr = reinterpret_cast<const char*>(funcraw);
		const char* safepointip = funcentryaddr + rd->LabelOffset;

		std::cout << (void*)(safepointip) << std::endl;
	}
}




