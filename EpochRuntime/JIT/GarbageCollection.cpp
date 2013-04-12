//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Garbage collection integrated into JIT native code generation layer
//
// Epoch implements a simple mark/sweep collector, which runs on each isolated
// "task" in the runtime. Since tasks may not share state, collection for each
// individual task can be relatively lightweight and does not need to stop the
// execution of any other running code in order to collect safely.
//
// The only exception is during the handoff of data between tasks (via message
// passing functionality) which we can easily handle using lock-free methods.
//

#include "pch.h"

#include "Virtual Machine/VirtualMachine.h"

#include "User Interface/Output.h"

#include "Utility/Strings.h"


// LLVM is naughty and generates lots of warnings.
#pragma warning(push)
#pragma warning(disable: 4100)		// Unreferenced parameter
#pragma warning(disable: 4244)		// Type conversion without explicit cast
#pragma warning(disable: 4512)		// Cannot generate assignment operator
#pragma warning(disable: 4624)		// Cannot generate destructor
#pragma warning(disable: 4800)		// Forcing coercion to bool

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


// TODO - centralize execution context
extern Runtime::ExecutionContext* GlobalContext;


using namespace llvm;

namespace
{
	//
	// This container holds a list of all GC-enabled functions generated
	// during machine code emission. We track this because LLVM does not
	// expose that list to us in any useful fashion, and we need to hold
	// on to the GC info for mapping safe points and accessing the stack
	// maps of live GC roots in running functions.
	//
	std::vector<GCFunctionInfo*> FunctionList;


	//
	// This structure represents a callstack frame in running code.
	// We use it for crawling the stack and finding live GC roots.
	//
	struct CallFrame
	{
		CallFrame* prevFrame;
		void* returnAddr;
	};

	//
	// Helper container for storing off a list of live GC roots
	// of various sorts: strings, buffers, other internal types
	// which are garbage collected, and structures of all kinds
	// including template instantiations.
	//
	struct LiveValues
	{
		boost::unordered_set<StringHandle> LiveStrings;
		boost::unordered_set<BufferHandle> LiveBuffers;
		boost::unordered_set<StructureHandle> LiveStructures;
	};


	//
	// Check a GC root to see if it should be added to the list
	// of live handles to traverse later.
	//
	void CheckRoot(const void* liveptr, Metadata::EpochTypeID type, LiveValues& livevalues)
	{
		switch(type)
		{
		case Metadata::EpochType_String:
			{
				StringHandle handle = *reinterpret_cast<const StringHandle*>(liveptr);
				if(handle)
					livevalues.LiveStrings.insert(handle);
			}
			break;

		case Metadata::EpochType_Buffer:
			{
				BufferHandle handle = *reinterpret_cast<const BufferHandle*>(liveptr);
				if(handle)
					livevalues.LiveBuffers.insert(handle);
			}
			break;

		default:
			{
				Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(type);

				// TODO - we do this "is structure or template instance" check a lot. Encapsulate?
				if(family == Metadata::EpochTypeFamily_Structure || family == Metadata::EpochTypeFamily_TemplateInstance)
				{
					StructureHandle handle = *reinterpret_cast<const StructureHandle*>(liveptr);
					if(handle)
						livevalues.LiveStructures.insert(handle);
				}
				else if(family == Metadata::EpochTypeFamily_SumType)
				{
					// TODO - implement sum type garbage collector tracing
					throw NotImplementedException("GC roots of sum types are not implemented");
				}
				else
					throw FatalException("Trying to garbage collect an unsupported data type");
			}
			break;
		}
	}

	//
	// Walk the stack for a given safe point and mark any GC roots found
	//
	void WalkLiveValuesForSafePoint(void* stackptr, GCFunctionInfo& funcinfo, GCFunctionInfo::iterator& safepoint, LiveValues& livevalues)
	{
		for(GCFunctionInfo::live_iterator LI = funcinfo.live_begin(safepoint), LE = funcinfo.live_end(safepoint); LI != LE; ++LI)
		{
			Metadata::EpochTypeID type = static_cast<Metadata::EpochTypeID>(dyn_cast<ConstantInt>(LI->Metadata->getOperand(0))->getValue().getLimitedValue());
			
			// TODO - this magic number is kind of scary!
			const char* liveptr = reinterpret_cast<const char*>(stackptr) - LI->StackOffset + 12;
			CheckRoot(liveptr, type, livevalues);
		}
	}

	//
	// Worker function for actually performing mark/sweep garbage collection
	//
	void GarbageWorker()
	{
		CallFrame* framePtr;
		__asm
		{
			mov framePtr, ebp
		}

		// TODO - don't collect if we haven't ticked over any collectors

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
					uint64_t modifiedaddress = address - 4;		// TODO - evil magic number
					if(reinterpret_cast<void*>(static_cast<unsigned>(address)) == returnAddr || reinterpret_cast<void*>(static_cast<unsigned>(modifiedaddress)) == returnAddr)
						WalkLiveValuesForSafePoint(framePtr, info, spiter, livevalues);
				}
			}

			framePtr = framePtr->prevFrame;
		}

		GlobalContext->OwnerVM.PrivateGetRawStringPool().GarbageCollect(livevalues.LiveStrings);

		// TODO - collect buffers
		// TODO - walk structure graph and collect any garbage not marked
	}


	//
	// Entry stub for invoking the garbage collector on a task
	//
	__declspec(naked) void TriggerGarbageCollection()
	{
		__asm
		{
			// Set up a stack frame for this function
			push ebp
			mov ebp, esp

			// Save the value of EAX so we can recover
			// the return value of the calling function
			// after garbage collection is over.
			push eax

			// Invoke GC
			call GarbageWorker

			// Restore EAX
			pop eax

			// Tear down stack frame
			mov esp, ebp
			pop ebp

			// Return to caller
			ret
		}
	}


	//
	// This class represents our integration with LLVM's GC support
	// infrastructure. It takes care of inserting GC safe points into
	// the code as well as caching data needed for retrieving the
	// stack maps later during actual collection passes.
	//
	class LLVM_LIBRARY_VISIBILITY EpochGC : public GCStrategy
	{
	// Construction
	public:

		//
		// Construct and initialize the GC strategy wrapper
		//
		EpochGC()
		{
			InitRoots = false;
			UsesMetadata = true;
			CustomRoots = true;
			CustomSafePoints = true;
			NeededSafePoints = 1 << GC::Return
							 | 1 << GC::PostCall;
		}

	// Interface overrides implementing GCStrategy functions
	public:

		//
		// Locate safe points in the code and inject labels for each
		//
		virtual bool findCustomSafePoints(GCFunctionInfo& FI, MachineFunction& MF)
		{
			// TODO - naming conventions (check whole file to be safe)
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

		//
		// We use a custom lowering pass to initialize GC roots to sane values
		// (i.e. zero) so that the GC doesn't traverse stack garbage. Note that
		// we can't use LLVM's version of this because it assumes all GC roots
		// are pointers (which is not true of, say, Epoch string handles).
		//
		virtual bool performCustomLowering(Function& function)
		{
			if(!function.hasGC())
				return false;

			bool modified = false;
			std::vector<AllocaInst*> roots;

			// Lower all GC read/write barriers and flag any roots in the function
			for(Function::iterator blockiter = function.begin(); blockiter != function.end(); ++blockiter)
			{
				for(BasicBlock::iterator instriter = blockiter->begin(); instriter != blockiter->end(); ++instriter)
				{
					IntrinsicInst* intrinsic = dyn_cast<IntrinsicInst>(instriter);
					if(intrinsic)
					{
						Function* calltarget = intrinsic->getCalledFunction();
						switch(calltarget->getIntrinsicID())
						{
						case Intrinsic::gcwrite:
							{
								Value* store = new StoreInst(intrinsic->getArgOperand(0), intrinsic->getArgOperand(2), intrinsic);
								intrinsic->replaceAllUsesWith(store);
								intrinsic->eraseFromParent();
							}
							break;

						case Intrinsic::gcread:
							{
								Value* load = new LoadInst(intrinsic->getArgOperand(1), "", intrinsic);
								load->takeName(intrinsic);
								intrinsic->replaceAllUsesWith(load);
								intrinsic->eraseFromParent();
							}
							break;
	
						case Intrinsic::gcroot:
							roots.push_back(cast<AllocaInst>(intrinsic->getArgOperand(0)->stripPointerCasts()));
							break;

						default:
							continue;
						}

						modified = true;
					}
				}
			}

			// Now initialize any roots we found in this function
			for(std::vector<AllocaInst*>::iterator allocaiter = roots.begin(); allocaiter != roots.end(); ++allocaiter)
			{
				StoreInst* store = NULL;

				if((*allocaiter)->getType()->getElementType()->isPointerTy())
					store = new StoreInst(ConstantPointerNull::get(cast<PointerType>(cast<PointerType>((*allocaiter)->getType())->getElementType())), *allocaiter);
				else
					store = new StoreInst(ConstantInt::get((*allocaiter)->getType()->getElementType(), 0), *allocaiter);

				store->insertAfter(*allocaiter);
				modified = true;
			}

			return modified;
		}

	// Internal GC strategy helpers
	private:

		//
		// Visit a call instruction and add safe points as needed
		//
		void VisitCallPoint(GCFunctionInfo& functioninfo, MachineBasicBlock::iterator instriter, const TargetInstrInfo* targetinfo)
		{
			MachineBasicBlock::iterator retinstr = instriter;
			++retinstr;

			if(needsSafePoint(GC::PreCall))
			{
				MCSymbol* label = InsertLabel(*instriter->getParent(), instriter, instriter->getDebugLoc(), targetinfo);
				functioninfo.addSafePoint(GC::PreCall, label, instriter->getDebugLoc());
			}

			if(needsSafePoint(GC::PostCall))
			{
				MCSymbol* label = InsertLabel(*instriter->getParent(), retinstr, instriter->getDebugLoc(), targetinfo);
				functioninfo.addSafePoint(GC::PostCall, label, instriter->getDebugLoc());
			}
		}

		//
		// Visit a return instruction and add safe points as needed
		//
		void VisitRet(GCFunctionInfo& functioninfo, MachineBasicBlock::iterator instriter, const TargetInstrInfo* targetinfo)
		{
			MCSymbol* label = InsertLabel(*instriter->getParent(), instriter, instriter->getDebugLoc(), targetinfo);
			functioninfo.addSafePoint(GC::Return, label, instriter->getDebugLoc());
			InsertGarbageCheck(*instriter->getParent(), instriter, instriter->getDebugLoc(), targetinfo);
		}


		//
		// Helper for inserting a machine code label into the instruction stream
		//
		MCSymbol* InsertLabel(MachineBasicBlock& block, MachineBasicBlock::iterator instriter, DebugLoc loc, const TargetInstrInfo* targetinfo) const
		{
			MCSymbol* label = block.getParent()->getContext().CreateTempSymbol();
			BuildMI(block, instriter, loc, targetinfo->get(TargetOpcode::GC_LABEL)).addSym(label);
			return label;
		}

		//
		// Helper for inserting a garbage collection invocation
		// check into the instruction stream. Note that this will
		// only invoke the GC if one or more types are ticked over
		// and ready for immediate collection; otherwise the code
		// will bail early for minimal performance impact.
		//
		void InsertGarbageCheck(MachineBasicBlock& block, MachineBasicBlock::iterator instriter, DebugLoc loc, const TargetInstrInfo* targetinfo) const
		{
			// We need to back up three instruction slots: one for the
			// RET, one for the POP EBP (so we preserve the function's
			// stack frame), and one to insert in the correct location
			// in the instruction stream. This makes sure that we call
			// the GC with all the relevant context it needs.
			--instriter;
			--instriter;
			--instriter;

			// TODO - this is a dirty hack based on LLVM's defined opcode index for CALL
			const MCInstrDesc& instrdesc = targetinfo->get(350);

			BuildMI(block, instriter, loc, instrdesc).addImm(reinterpret_cast<unsigned>(&TriggerGarbageCollection));
		}
	};


	//
	// This ensures that the GC strategy is instantiated and added to the LLVM registry
	//
	GCRegistry::Add<EpochGC> EpochGCInst("EpochGC", "Epoch Programming Language Garbage Collector");
}


//
// External API for cleaning up the GC info list.
//
// Primarily useful for running the same DLL instance
// against several programs, e.g. when running a unit
// test suite.
//
void ClearGCContextInfo()
{
	FunctionList.clear();
}

