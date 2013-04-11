#include "pch.h"


// LLVM is naughty and generates lots of warnings.
#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4244)
#pragma warning(disable: 4512)
#pragma warning(disable: 4624)
#pragma warning(disable: 4800)

#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/GCMetadataPrinter.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/IntrinsicInst.h"

#pragma warning(pop)


#include "Virtual Machine/VirtualMachine.h"
#include "User Interface/Output.h"
#include "Utility/Strings.h"


extern "C" void VMGarbageCollect();

extern VM::ExecutionContext* GlobalContext;


using namespace llvm;

namespace
{
	std::vector<GCFunctionInfo*> FunctionList;

	MCContext* MyMCContext = NULL;


bool CouldBecomeSafePoint(Instruction *I) {
  // The natural definition of instructions which could introduce safe points
  // are:
  //
  //   - call, invoke (AfterCall, BeforeCall)
  //   - phis (Loops)
  //   - invoke, ret, unwind (Exit)
  //
  // However, instructions as seemingly inoccuous as arithmetic can become
  // libcalls upon lowering (e.g., div i64 on a 32-bit platform), so instead
  // it is necessary to take a conservative approach.

  if (isa<AllocaInst>(I) || isa<GetElementPtrInst>(I) ||
      isa<StoreInst>(I) || isa<LoadInst>(I))
    return false;

  // llvm.gcroot is safe because it doesn't do anything at runtime.
  if (CallInst *CI = dyn_cast<CallInst>(I))
    if (Function *F = CI->getCalledFunction())
      if (unsigned IID = F->getIntrinsicID())
        if (IID == Intrinsic::gcroot)
          return false;

  return true;
}


	
	class LLVM_LIBRARY_VISIBILITY EpochGC : public GCStrategy
	{
	public:
		EpochGC()
		{
			InitRoots = false;
			UsesMetadata = true;
			CustomRoots = true;
			CustomSafePoints = true;
			NeededSafePoints = 1 << GC::Return
							 | 1 << GC::PostCall;
		}

		virtual bool findCustomSafePoints(GCFunctionInfo& FI, MachineFunction& MF)
		{
			for(MachineFunction::iterator BBI = MF.begin(), BBE = MF.end(); BBI != BBE; ++BBI)
			{
				for(MachineBasicBlock::iterator MI = BBI->begin(), ME = BBI->end(); MI != ME; ++MI)
				{
					if(MI->isCall())
						VisitCallPoint(FI, MI, MF.getTarget().getInstrInfo());
					else if(MI->isReturn())
						VisitRet(FI, MI, MF.getTarget().getInstrInfo());
				}
			}

			FunctionList.push_back(&FI);

			return true;
		}

		virtual bool performCustomLowering(Function& F)
		{
			if(!F.hasGC())
				return false;

			bool MadeChange = false;
			std::vector<AllocaInst*> Roots;

			for(Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
			{
				for(BasicBlock::iterator II = BB->begin(), E = BB->end(); II != E;)
				{
					if(IntrinsicInst *CI = dyn_cast<IntrinsicInst>(II++))
					{
						Function *F = CI->getCalledFunction();
						
						switch (F->getIntrinsicID())
						{
						case Intrinsic::gcwrite:
							{
								// Replace a write barrier with a simple store.
								Value *St = new StoreInst(CI->getArgOperand(0),
								CI->getArgOperand(2), CI);
								CI->replaceAllUsesWith(St);
								CI->eraseFromParent();
							}
							break;

						case Intrinsic::gcread:
							{
								// Replace a read barrier with a simple load.
								Value *Ld = new LoadInst(CI->getArgOperand(1), "", CI);
								Ld->takeName(CI);
								CI->replaceAllUsesWith(Ld);
								CI->eraseFromParent();
							}
							break;
	
						case Intrinsic::gcroot:
							{
								// Initialize the GC root, but do not delete the intrinsic. The
								// backend needs the intrinsic to flag the stack slot.
								Roots.push_back(cast<AllocaInst>(CI->getArgOperand(0)->stripPointerCasts()));
							}
							break;

						default:
							continue;
						}

						MadeChange = true;
					}
				}
			}

			if(!Roots.empty())
			{
				for(std::vector<AllocaInst*>::iterator allocaiter = Roots.begin(); allocaiter != Roots.end(); ++allocaiter)
				{
					StoreInst* SI = NULL;

					if((*allocaiter)->getType()->getElementType()->isPointerTy())
						SI = new StoreInst(ConstantPointerNull::get(cast<PointerType>(cast<PointerType>((*allocaiter)->getType())->getElementType())), *allocaiter);
					else
						SI = new StoreInst(ConstantInt::get((*allocaiter)->getType()->getElementType(), 0), *allocaiter);

					SI->insertAfter(*allocaiter);
				}
			}
			
			return MadeChange;
		}

	private:
		void VisitCallPoint(GCFunctionInfo& FI, MachineBasicBlock::iterator CI, const TargetInstrInfo* TII)
		{
			MachineBasicBlock::iterator RAI = CI;
			++RAI;

			if(needsSafePoint(GC::PreCall))
			{
				MCSymbol* Label = InsertLabel(*CI->getParent(), CI, CI->getDebugLoc(), TII);
				FI.addSafePoint(GC::PreCall, Label, CI->getDebugLoc());
			}

			if(needsSafePoint(GC::PostCall))
			{
				MCSymbol* Label = InsertLabel(*CI->getParent(), RAI, CI->getDebugLoc(), TII);
				FI.addSafePoint(GC::PostCall, Label, CI->getDebugLoc());
			}
		}

		void VisitRet(GCFunctionInfo& FI, MachineBasicBlock::iterator CI, const TargetInstrInfo* TII)
		{
			MCSymbol* label = InsertLabel(*CI->getParent(), CI, CI->getDebugLoc(), TII);
			FI.addSafePoint(GC::Return, label, CI->getDebugLoc());
			InsertGarbageCheck(*CI->getParent(), CI, CI->getDebugLoc(), TII);
		}

		MCSymbol* InsertLabel(MachineBasicBlock& MBB, MachineBasicBlock::iterator MI, DebugLoc DL, const TargetInstrInfo* TII) const
		{
			MyMCContext = &MBB.getParent()->getContext();
			MCSymbol* Label = MBB.getParent()->getContext().CreateTempSymbol();
			BuildMI(MBB, MI, DL, TII->get(TargetOpcode::GC_LABEL)).addSym(Label);
			return Label;
		}

		void InsertGarbageCheck(MachineBasicBlock& MBB, MachineBasicBlock::iterator MI, DebugLoc DL, const TargetInstrInfo* TII) const
		{
			// TODO - this is a dirty hack based on LLVM's defined opcode index for CALL
			// Also note that we dirtily hack back an instruction offset so we precede the POP EPB instruction
			// and the stack cleanup that precedes it.
			--MI;
			--MI;
			--MI;
			BuildMI(MBB, MI, DL, TII->get(350)).addImm(reinterpret_cast<unsigned>(&VMGarbageCollect));
		}
	};

	GCRegistry::Add<EpochGC> EpochGCInst("EpochGC", "Epoch Programming Language Garbage Collector");



	void DumpInfoForFunction(GCFunctionInfo& FI)
	{
		std::wstring Symbol;
		Symbol += L"__gcmap_";
		Symbol += widen(FI.getFunction().getName());

		UI::OutputStream out;
		out << Symbol << L" has " << FI.size() << L" safe points" << std::endl;

		for(GCFunctionInfo::iterator PI = FI.begin(), PE = FI.end(); PI != PE; ++PI)
		{
			out << L"Safe point: label" << PI->Label->getName().data() << std::endl;
			out << L"Frame size: " << FI.getFrameSize() << std::endl;
			out << L"Live roots: " << FI.live_size(PI) << std::endl;

			for(GCFunctionInfo::live_iterator LI = FI.live_begin(PI), LE = FI.live_end(PI); LI != LE; ++LI)
				out << L"Root offset: " << LI->StackOffset << std::endl;
		}

		out << std::endl;
	}

}


void DumpGCInfo()
{
	for(std::vector<GCFunctionInfo*>::iterator iter = FunctionList.begin(); iter != FunctionList.end(); ++iter)
	{
		DumpInfoForFunction(**iter);
	}
}




struct CallFrame
{
	CallFrame* prevFrame;
	void* returnAddr;
};


struct LiveValues
{
	boost::unordered_set<StringHandle> LiveStrings;
};


void WalkLiveValuesForSafePoint(void* stackptr, GCFunctionInfo& funcinfo, GCFunctionInfo::iterator& safepoint, LiveValues& livevalues)
{
	for(GCFunctionInfo::live_iterator LI = funcinfo.live_begin(safepoint), LE = funcinfo.live_end(safepoint); LI != LE; ++LI)
	{
		if(dyn_cast<ConstantInt>(LI->Metadata->getOperand(0))->getValue().getLimitedValue() == Metadata::EpochType_String)
		{
			const char* liveptr = reinterpret_cast<const char*>(stackptr) - LI->StackOffset + 12;
			StringHandle handle = *reinterpret_cast<const StringHandle*>(liveptr);

			if(handle)
				livevalues.LiveStrings.insert(handle);
		}
	}
}


void GarbageWorker()
{
	CallFrame* framePtr;
	__asm
	{
		mov framePtr, ebp
	}

	LiveValues livevalues;
	livevalues.LiveStrings = GlobalContext->StaticallyReferencedStrings;

	while(framePtr != NULL)
	{
		void* returnAddr = framePtr->returnAddr;
		for(std::vector<GCFunctionInfo*>::iterator iter = FunctionList.begin(); iter != FunctionList.end(); ++iter)
		{
			GCFunctionInfo& info = **iter;
			for(GCFunctionInfo::iterator spiter = info.begin(); spiter != info.end(); ++spiter)
			{
				uint64_t address = reinterpret_cast<ExecutionEngine*>(GlobalContext->OwnerVM.JITExecutionEngine)->getLabelAddress(spiter->Label);
				uint64_t modifiedaddress = address - 4;
				if(reinterpret_cast<void*>(static_cast<unsigned>(address)) == returnAddr || reinterpret_cast<void*>(static_cast<unsigned>(modifiedaddress)) == returnAddr)
					WalkLiveValuesForSafePoint(framePtr, info, spiter, livevalues);
			}
		}

		framePtr = framePtr->prevFrame;
	}

	GlobalContext->OwnerVM.PrivateGetRawStringPool().GarbageCollect(livevalues.LiveStrings);
}


extern "C" __declspec(naked) void VMGarbageCollect()
{
	__asm
	{
		push ebp
		mov ebp, esp

		push eax
		call GarbageWorker
		pop eax

		mov esp, ebp
		pop ebp
		ret
	}
}
