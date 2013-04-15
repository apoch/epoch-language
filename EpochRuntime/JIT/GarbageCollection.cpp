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

#include "Runtime/Runtime.h"
#include "Runtime/GlobalContext.h"

#include "JIT/GarbageCollection.h"

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


	std::map<const Function*, std::pair<void*, size_t> > FunctionBounds;


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

		// Bitmask built from Runtime::ExecutionContext::GarbageCollectionFlags
		unsigned Mask;
	};


	//
	// Check a GC root to see if it should be added to the list
	// of live handles to traverse later.
	//
	void CheckRoot(const void* liveptr, Metadata::EpochTypeID type, LiveValues& livevalues)
	{
		if(!liveptr)
			return;

		switch(type)
		{
		case Metadata::EpochType_String:
			if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Strings)
			{
				StringHandle handle = *reinterpret_cast<const StringHandle*>(liveptr);
				if(handle)
					livevalues.LiveStrings.insert(handle);
			}
			break;

		case Metadata::EpochType_Buffer:
			if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Buffers)
			{
				BufferHandle handle = *reinterpret_cast<const BufferHandle*>(liveptr);
				if(handle)
					livevalues.LiveBuffers.insert(handle);
			}
			break;

		default:
			{
				if(Metadata::IsStructureType(type))
				{
					if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Structures)
					{
						StructureHandle handle = *reinterpret_cast<const StructureHandle*>(liveptr);
						if(handle)
							livevalues.LiveStructures.insert(handle);
					}
				}
				else if(Metadata::GetTypeFamily(type) == Metadata::EpochTypeFamily_SumType)
				{
					Metadata::EpochTypeID realtype = *reinterpret_cast<const Metadata::EpochTypeID*>(liveptr);
					if(Metadata::GetTypeFamily(realtype) == Metadata::EpochTypeFamily_SumType)
					{
						CheckRoot(*reinterpret_cast<void* const*>(reinterpret_cast<const char*>(liveptr) + sizeof(Metadata::EpochTypeID)), realtype, livevalues);
					}
					else
						CheckRoot(reinterpret_cast<const char*>(liveptr) + sizeof(Metadata::EpochTypeID), realtype, livevalues);
				}
				else if(type == Metadata::EpochType_Nothing)
				{
					// Nothing to do!
				}
			}
			break;
		}
	}

	//
	// Walk the stack for a given safe point and mark any GC roots found
	//
	void WalkLiveValuesForSafePoint(void* prevframeptr, void* stackptr, GCFunctionInfo& funcinfo, GCFunctionInfo::iterator& safepoint, LiveValues& livevalues)
	{
		for(GCFunctionInfo::live_iterator LI = funcinfo.live_begin(safepoint), LE = funcinfo.live_end(safepoint); LI != LE; ++LI)
		{
			Metadata::EpochTypeID type = static_cast<Metadata::EpochTypeID>(dyn_cast<ConstantInt>(LI->Metadata->getOperand(0))->getValue().getLimitedValue());
			if(type == 0xffffffff)
				continue;
			
			const char* liveptr;
			if(static_cast<signed>(LI->StackOffset) <= 0)
				liveptr = reinterpret_cast<const char*>(stackptr) + LI->StackOffset;
			else
				liveptr = reinterpret_cast<const char*>(prevframeptr) + LI->StackOffset + sizeof(CallFrame);

			CheckRoot(liveptr, type, livevalues);
		}
	}

	//
	// Helper to traverse the heap graph of reachable structures
	//
	void WalkStructuresForLiveHandles(LiveValues& livevalues)
	{
		bool addedhandle;

		// TODO - this is a naive traversal and rather slow.
		do
		{
			addedhandle = false;

			for(boost::unordered_set<StructureHandle>::const_iterator iter = livevalues.LiveStructures.begin(); iter != livevalues.LiveStructures.end(); ++iter)
			{
				const ActiveStructure& active = Runtime::GetThreadContext()->FindStructureMetadata(*iter);
				const StructureDefinition& def = active.Definition;
				for(size_t i = 0; i < def.GetNumMembers(); ++i)
				{
					Metadata::EpochTypeID membertype = def.GetMemberType(i);
					if(Metadata::IsStructureType(membertype) && (livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Structures))
					{
						StructureHandle p = active.ReadMember<StructureHandle>(i);
						if(livevalues.LiveStructures.count(p) == 0)
						{
							livevalues.LiveStructures.insert(p);
							addedhandle = true;
						}
					}
					else if(membertype == Metadata::EpochType_String)
					{
						if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Strings)
						{
							StringHandle h = active.ReadMember<StringHandle>(i);
							livevalues.LiveStrings.insert(h);
						}
					}
					else if(membertype == Metadata::EpochType_Buffer)
					{
						if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Buffers)
						{
							BufferHandle h = active.ReadMember<BufferHandle>(i);
							livevalues.LiveBuffers.insert(h);
						}
					}
					else if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_SumType)
					{
						Metadata::EpochTypeID realtype = active.ReadSumTypeMemberType(i);
						if(Metadata::IsStructureType(realtype) && (livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Structures))
						{
							StructureHandle p = active.ReadMember<StructureHandle>(i);
							if(livevalues.LiveStructures.count(p) == 0)
							{
								livevalues.LiveStructures.insert(p);
								addedhandle = true;
							}
						}
						else if(realtype == Metadata::EpochType_String)
						{
							if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Strings)
							{
								StringHandle h = active.ReadMember<StringHandle>(i);
								livevalues.LiveStrings.insert(h);
							}
						}
						else if(realtype == Metadata::EpochType_Buffer)
						{
							if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Buffers)
							{
								BufferHandle h = active.ReadMember<BufferHandle>(i);
								livevalues.LiveBuffers.insert(h);
							}
						}
					}
				}

				if(addedhandle)
					break;
			}
		} while(addedhandle);
	}

	//
	// Worker function for actually performing mark/sweep garbage collection
	//
	void GarbageWorker(char* rawptr)
	{
		unsigned collectmask = Runtime::GetThreadContext()->GetGarbageCollectionBitmask();
		if(!collectmask)
			return;

		LiveValues livevalues;
		livevalues.Mask = collectmask;

		CallFrame* framePtr = reinterpret_cast<CallFrame*>(rawptr + sizeof(void*));
		void* prevFramePtr = NULL;

		bool foundany = false;
		while(framePtr != NULL)
		{
			void* returnAddr = framePtr->returnAddr;
			prevFramePtr = framePtr;
			framePtr = framePtr->prevFrame;

			bool found = false;
			for(std::vector<GCFunctionInfo*>::iterator iter = FunctionList.begin(); iter != FunctionList.end(); ++iter)
			{
				GCFunctionInfo& info = **iter;

				std::pair<void*, size_t> bounds = FunctionBounds.find(&info.getFunction())->second;
				const void* boundend = reinterpret_cast<const char*>(bounds.first) + bounds.second;

				if(returnAddr < bounds.first || returnAddr > boundend)
					continue;

				for(GCFunctionInfo::iterator spiter = info.begin(); spiter != info.end(); ++spiter)
				{
					uint64_t address = reinterpret_cast<ExecutionEngine*>(Runtime::GetThreadContext()->JITExecutionEngine)->getLabelAddress(spiter->Label);
					const void* addressptr = reinterpret_cast<void*>(static_cast<unsigned>(address));

					if(addressptr == returnAddr)
					{
						WalkLiveValuesForSafePoint(prevFramePtr, framePtr, info, spiter, livevalues);
						found = foundany = true;
					}
				}
			}

			if(!found)
				break;
		}

		if(!foundany)
			return;

		WalkStructuresForLiveHandles(livevalues);

		if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Strings)
			Runtime::GetThreadContext()->PrivateGetRawStringPool().GarbageCollect(livevalues.LiveStrings, Runtime::GetThreadContext()->StaticallyReferencedStrings);

		if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Buffers)
			Runtime::GetThreadContext()->GarbageCollectBuffers(livevalues.LiveBuffers);

		if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Structures)
			Runtime::GetThreadContext()->GarbageCollectStructures(livevalues.LiveStructures);
	}


	//
	// This class represents our integration with LLVM's GC support
	// infrastructure. It takes care of inserting GC safe points into
	// the code as well as caching data needed for retrieving the
	// stack maps later during actual collection passes.
	//
	class LLVM_LIBRARY_VISIBILITY EpochGCStrategy : public GCStrategy
	{
	// Construction
	public:

		//
		// Construct and initialize the GC strategy wrapper
		//
		EpochGCStrategy()
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
			if(!FI.getFunction().hasGC())
				return false;

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
				if((*allocaiter)->getType()->getElementType()->isPointerTy())
				{
					StoreInst* store = new StoreInst(ConstantPointerNull::get(cast<PointerType>(cast<PointerType>((*allocaiter)->getType())->getElementType())), *allocaiter);
					store->insertAfter(*allocaiter);
				}
				else if((*allocaiter)->getType()->getElementType()->isAggregateType())
				{
					BitCastInst* castptr = new BitCastInst(*allocaiter, Type::getInt32PtrTy(getGlobalContext()));
					castptr->insertAfter(*allocaiter);
					StoreInst* store = new StoreInst(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0), castptr);
					store->insertAfter(castptr);

					std::vector<Value*> indices;
					indices.push_back(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0));
					indices.push_back(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 1));
					GetElementPtrInst* gep = GetElementPtrInst::Create(*allocaiter, indices);
					gep->insertAfter(store);

					if(gep->getType()->getElementType()->isPointerTy())
					{
						PointerType* ptype = cast<PointerType>(cast<PointerType>(gep->getType())->getElementType());
						StoreInst* payloadstore = new StoreInst(ConstantPointerNull::get(ptype), gep);
						payloadstore->insertAfter(gep);
					}
					else
					{
						StoreInst* payloadstore = new StoreInst(ConstantInt::get(gep->getType()->getElementType(), 0), gep);
						payloadstore->insertAfter(gep);
					}
				}
				else
				{
					StoreInst* store = new StoreInst(ConstantInt::get((*allocaiter)->getType()->getElementType(), 0), *allocaiter);
					store->insertAfter(*allocaiter);
				}

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
	};


	//
	// This ensures that the GC strategy is instantiated and added to the LLVM registry
	//
	GCRegistry::Add<EpochGCStrategy> EpochGCInst("EpochGC", "Epoch Programming Language Garbage Collector");
}


//
// External API for cleaning up the GC info list.
//
// Primarily useful for running the same DLL instance
// against several programs, e.g. when running a unit
// test suite.
//
void EpochGC::ClearGCContextInfo()
{
	FunctionList.clear();
}


void EpochGC::SetGCFunctionBounds(const Function* func, void* start, size_t size)
{
	FunctionBounds[func] = std::make_pair(start, size);
}



//
// Entry stub for invoking the garbage collector on a task
//
extern "C" __declspec(naked) void TriggerGarbageCollection()
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
		push esp
		call GarbageWorker
		add esp, 4

		// Restore EAX
		pop eax

		// Tear down stack frame
		mov esp, ebp
		pop ebp

		// Return to caller
		ret
	}
}
