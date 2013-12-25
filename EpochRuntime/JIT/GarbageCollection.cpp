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

#include <iostream>


// LLVM is naughty and generates lots of warnings.
#pragma warning(push)
#pragma warning(disable: 4100)		// Unreferenced parameter
#pragma warning(disable: 4244)		// Type conversion without explicit cast
#pragma warning(disable: 4510)		// Cannot generate constructor
#pragma warning(disable: 4610)		// Cannot generate constructor
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
#include "llvm/IR/IntrinsicInst.h"

#pragma warning(pop)


using namespace llvm;

namespace
{
	// TODO - document
	__declspec(thread) std::map<const Function*, GCFunctionInfo*>* FunctionGCInfoCache = NULL;

	typedef boost::unordered_map<const void*, std::pair<GCFunctionInfo*, GCFunctionInfo::iterator> > SafePointCacheT;
	__declspec(thread) SafePointCacheT* SafePointCache = NULL;

	//
	// Map of all allocated global variables (their in-memory locations)
	// to the type metadata associated with the global. This ought to be
	// turned into a thread-local storage container instead.
	//
	__declspec(thread) std::map<void*, Metadata::EpochTypeID>* Globals = NULL;


	//
	// This structure represents a callstack frame in running code.
	// We use it for crawling the stack and finding live GC roots.
	//
	struct CallFrame
	{
		CallFrame* PreviousFrame;
		void* ReturnAddress;
	};

	//
	// Helper container for storing off a list of live GC roots
	// of various sorts: strings, buffers, other internal types
	// which are garbage collected, and structures of all kinds
	// including template instantiations.
	//
	struct LiveValues
	{
		std::set<StringHandle> LiveStrings;
		std::set<BufferHandle> LiveBuffers;
		std::set<StructureHandle> LiveStructures;

		//
		// Bitmask built from Runtime::ExecutionContext::GarbageCollectionFlags
		//
		// We don't garbage collect every possible type every time we run the
		// mark/sweep logic. Instead, a counter is used to indicate when "too
		// many" allocations have been made in a specific memory category.
		//
		// Once this counter hits the defined threshold (which should be user
		// configurable), the next GC invocation will sweep for objects which
		// have the appropriate type/type family. Doing this allows GC invoke
		// calls to be made frequently at little real expense, since we early
		// exit if the mask indicates nothing is ready to be cleaned up.
		//
		unsigned Mask;
	};


	//
	// Bookmarks are used to maintain stable stack tracing in the presence
	// of interop with external functions that have been compiled with the
	// Frame Pointer Omission optimization active. FPO causes some entries
	// on the call stack to be "invisible" to our stack walking routines.
	//
	// To counteract this, whenever we invoke an external function from an
	// Epoch routine, we stash a "bookmark" in a heap-allocated container.
	// Each bookmark describes the last stack frame that we observed under
	// Epoch runtime control. During stack tracing, we also check for live
	// roots in each stack frame described by a bookmark.
	//
	// This allows Epoch routines to call into external code compiled with
	// FPO, which then invoke Epoch code via marshaled a callback, without
	// losing track of the stack areas that need to be traversed for roots
	// in the garbage collection process.
	//
	struct BookmarkEntry
	{
		void* StackPointer;
		void* ReturnAddress;
	};

	//
	// This container holds all active bookmarks.
	//
	// It should be moved to be held in a task's context instead.
	//
	__declspec(thread) std::vector<BookmarkEntry>* Bookmarks = NULL;


	//
	// Check a GC root to see if it should be added to the list
	// of live handles to traverse later.
	//
	void CheckRoot(const void* liveptr, Metadata::EpochTypeID type, LiveValues& livevalues)
	{
		// Never traverse null pointers (this may occur for sum
		// types and other references to live objects via recursive
		// calls to CheckRoot).
		if(!liveptr)
			return;

		switch(type)
		{
		//
		// Mark a string handle as active, if strings are currently
		// in the set of types being garbage collected. See comment
		// on LiveValues::Mask for details.
		//
		case Metadata::EpochType_String:
			if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Strings)
			{
				StringHandle handle = *reinterpret_cast<const StringHandle*>(liveptr);
				if(handle)
					livevalues.LiveStrings.insert(handle);
			}
			break;

		//
		// Mark a buffer handle as active, if appropriate.
		//
		case Metadata::EpochType_Buffer:
			if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Buffers)
			{
				BufferHandle handle = *reinterpret_cast<const BufferHandle*>(liveptr);
				if(handle)
					livevalues.LiveBuffers.insert(handle);
			}
			break;

		//
		// Handle the general case
		//
		default:
			{
				//
				// Check for structures first, they're most common.
				//
				// After that, check for algebraic sum types and possibly
				// recursively invoke CheckRoot to retrieve the real root
				// based on the attached type annotation.
				//
				if(Metadata::IsStructureType(type))
				{
					StructureHandle handle = *reinterpret_cast<const StructureHandle*>(liveptr);
					if(handle)
						livevalues.LiveStructures.insert(handle);
				}
				else if(Metadata::GetTypeFamily(type) == Metadata::EpochTypeFamily_SumType)
				{
					//
					// Algebraic sum types are represented as a type annotation followed
					// by an arbitrary-length byte field. To trace them properly we must
					// extract the type signature, determine what type is currently held
					// by the sum-typed variable, and then if appropriate mark that data
					// as a live handle. To accomplish this, we simply read off the type
					// signature, and then recursively invoke CheckRoot with an adjusted
					// pointer to the payload of the sum-typed variable and the new type
					// data retrieved from the signature field.
					//
					// In the event that a sum-typed variable holds a nested object of a
					// sum type, we must dereference the adjusted pointer. Otherwise, we
					// can simply pass along that pointer to the recursive call.
					//
					Metadata::EpochTypeID realtype = *reinterpret_cast<const Metadata::EpochTypeID*>(liveptr);
					const void* adjustedptr = reinterpret_cast<const char*>(liveptr) + sizeof(Metadata::EpochTypeID);

					if(Metadata::GetTypeFamily(realtype) == Metadata::EpochTypeFamily_SumType)
						CheckRoot(*reinterpret_cast<void* const*>(adjustedptr), realtype, livevalues);
					else
						CheckRoot(adjustedptr, realtype, livevalues);
				}

				//
				// Why don't we handle the general case where a type is not GC-able?
				//
				// This is a sticky tradeoff. It would be nice to have a sanity check
				// here which catches the case where we traverse a bogus root and end
				// up with an invalid type signature, or the signature of a type that
				// we can't garbage collect. However, because of algebraic sum types,
				// we might actually be invoked with a type signature that is not one
				// of the collectible types. So we just ignore that case silently.
				//
				// One alternative would be to pass a flag to CheckRoot() which tells
				// the function whether or not to consider that case an error.
				//
			}
			break;
		}
	}

	//
	// Walk the stack for a given safe point and mark any GC roots found
	//
	void WalkLiveValuesForSafePoint(const void* calleeframeptr, const void* stackptr, GCFunctionInfo& funcinfo, GCFunctionInfo::iterator& safepoint, LiveValues& livevalues)
	{
		for(GCFunctionInfo::live_iterator liveiter = funcinfo.live_begin(safepoint); liveiter != funcinfo.live_end(safepoint); ++liveiter)
		{
			// Extract the type annotation from the root's metadata
			Metadata::EpochTypeID type = static_cast<Metadata::EpochTypeID>(dyn_cast<ConstantInt>(liveiter->Metadata->getOperand(0))->getValue().getLimitedValue());

			// Determine how to interpret the offset and compute the
			// memory location of the actual (potentially) live root
			const char* liveptr;
			if(static_cast<signed>(liveiter->StackOffset) <= 0)
				liveptr = reinterpret_cast<const char*>(stackptr) + liveiter->StackOffset;
			else
				liveptr = reinterpret_cast<const char*>(calleeframeptr) + liveiter->StackOffset + 8;

			// Ignore pointers to their own location on the stack.
			// This prevents a (rare) bug from occurring.
			// TODO - revisit this hack and see if it is still necessary without stack slot merging
			if(*reinterpret_cast<void* const*>(liveptr) == liveptr)
				continue;

			CheckRoot(liveptr, type, livevalues);
		}
	}

	//
	// Helper for visiting a single structure in the heap
	//
	void VisitStructure(StructureHandle handle, LiveValues& livevalues, Runtime::ExecutionContext* context, std::set<StructureHandle>& newhandles)
	{
		const ActiveStructure& active = context->FindStructureMetadata(handle);
		const StructureDefinition& def = active.Definition;
		for(size_t i = 0; i < def.GetNumMembers(); ++i)
		{
			Metadata::EpochTypeID membertype = def.GetMemberType(i);
			if(Metadata::IsStructureType(membertype))
			{
				StructureHandle p = active.ReadMember<StructureHandle>(i);
				newhandles.insert(p);
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
				if(Metadata::IsStructureType(realtype))
				{
					StructureHandle p = active.ReadMember<StructureHandle>(i);
					newhandles.insert(p);
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
	}

	//
	// Helper to traverse the heap graph of reachable structures
	//
	void WalkStructuresForLiveHandles(LiveValues& livevalues, Runtime::ExecutionContext* context)
	{
		std::set<StructureHandle>* valuestowalk = &livevalues.LiveStructures;

		bool addedhandle;
		std::set<StructureHandle> newhandles1;
		std::set<StructureHandle> newhandles2;

		std::set<StructureHandle>* newhandlebuffer = &newhandles1;

		do
		{
			for(std::set<StructureHandle>::const_iterator iter = valuestowalk->begin(); iter != valuestowalk->end(); ++iter)
				VisitStructure(*iter, livevalues, context, *newhandlebuffer);

			addedhandle = !newhandlebuffer->empty();
			if(addedhandle)
			{
				for(std::set<StructureHandle>::const_iterator iter = newhandlebuffer->begin(); iter != newhandlebuffer->end(); ++iter)
					livevalues.LiveStructures.insert(*iter);

				valuestowalk = newhandlebuffer;
				if(newhandlebuffer == &newhandles1)
					newhandlebuffer = &newhandles2;
				else
					newhandlebuffer = &newhandles1;
				newhandlebuffer->clear();
			}
		} while(addedhandle);
	}

	//
	// Traverse the list of global variables and mark any GC roots
	//
	void WalkGlobalsForLiveHandles(LiveValues& values)
	{
		if(!Globals)
			return;

		for(std::map<void*, Metadata::EpochTypeID>::const_iterator iter = Globals->begin(); iter != Globals->end(); ++iter)
			CheckRoot(iter->first, iter->second, values);
	}

	//
	// A counterpart to WalkLiveValuesForSafePoint, except using bookmarks.
	//
	// See the comments on the BookmarkEntry structure for details on how this
	// works and why it is implemented in the first place.
	//
	void WalkLiveValuesForBookmark(const void* stackptr, GCFunctionInfo& funcinfo, GCFunctionInfo::iterator& safepoint, LiveValues& livevalues)
	{
		for(GCFunctionInfo::live_iterator liveiter = funcinfo.live_begin(safepoint); liveiter != funcinfo.live_end(safepoint); ++liveiter)
		{
			Metadata::EpochTypeID type = static_cast<Metadata::EpochTypeID>(dyn_cast<ConstantInt>(liveiter->Metadata->getOperand(0))->getValue().getLimitedValue());

			//
			// Compute address of actual stack root
			//
			// Note that this works a bit differently than
			// the version in WalkLiveValuesForSafePoint.
			//
			const char* liveptr;
			if(static_cast<signed>(liveiter->StackOffset) <= 0)
				liveptr = reinterpret_cast<const char*>(stackptr) + liveiter->StackOffset;
			else
				liveptr = reinterpret_cast<const char*>(stackptr) - funcinfo.getFrameSize() + liveiter->StackOffset;

			// Skip pointers that point to themselves
			// TODO - remove if possible?
			if(*reinterpret_cast<void* const*>(liveptr) == liveptr)
				continue;

			CheckRoot(liveptr, type, livevalues);
		}
	}

	//
	// Traverse a bookmark entry and crawl its stack frame for roots
	//
	void WalkBookmarkEntry(const BookmarkEntry& entry, LiveValues& livevalues)
	{
		SafePointCacheT::iterator iter = SafePointCache->find(entry.ReturnAddress);
		if(iter != SafePointCache->end())
			WalkLiveValuesForBookmark(reinterpret_cast<char*>(entry.StackPointer), *(iter->second.first), iter->second.second, livevalues);
	}

	//
	// Walk the complete list of active stack bookmarks
	//
	bool WalkBookmarks(LiveValues& livevalues)
	{
		if(!Bookmarks || Bookmarks->empty())
			return false;

		for(std::vector<BookmarkEntry>::const_iterator iter = Bookmarks->begin(); iter != Bookmarks->end(); ++iter)
			WalkBookmarkEntry(*iter, livevalues);

		return true;
	}

	//
	// Worker function for crawling stack frames, and garbage collecting
	// anything that needs to be recycled based on the results of a mark
	// and sweep scan of stack roots and the heap pointer graph.
	//
	void CrawlStackAndGarbageCollect(const void* rawptr)
	{
		Runtime::ExecutionContext* context = Runtime::GetThreadContext();
		unsigned collectmask = context->GetGarbageCollectionBitmask();
		if(!collectmask || !SafePointCache)
			return;

		LiveValues livevalues;
		livevalues.Mask = collectmask;

		WalkGlobalsForLiveHandles(livevalues);

		const CallFrame* framepointer = reinterpret_cast<const CallFrame*>(rawptr);
		const void* calleeframepointer = NULL;

		bool foundany = false;
		while(framepointer != NULL)
		{
			const void* returnaddress = framepointer->ReturnAddress;
			calleeframepointer = framepointer;
			framepointer = framepointer->PreviousFrame;

			if(!framepointer)
				break;

			SafePointCacheT::iterator iter = SafePointCache->find(returnaddress);
			if(iter != SafePointCache->end())
			{
				WalkLiveValuesForSafePoint(calleeframepointer, framepointer, *(iter->second.first), iter->second.second, livevalues);
				foundany = true;
			}
		}

		foundany |= WalkBookmarks(livevalues);

		if(!foundany)
			return;

		WalkStructuresForLiveHandles(livevalues, context);

		if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Strings)
			context->PrivateGetRawStringPool().GarbageCollect(livevalues.LiveStrings, context->StaticallyReferencedStrings);

		if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Buffers)
			context->GarbageCollectBuffers(livevalues.LiveBuffers);

		if(livevalues.Mask & Runtime::ExecutionContext::GC_Collect_Structures)
			context->GarbageCollectStructures(livevalues.LiveStructures);
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
			NeededSafePoints = (1 << GC::Return) | (1 << GC::PostCall);
		}

	// Interface overrides implementing GCStrategy functions
	public:

		//
		// Locate safe points in the code and inject labels for each
		//
		virtual bool findCustomSafePoints(GCFunctionInfo& funcinfo, MachineFunction& machinefunc)
		{
			if(!funcinfo.getFunction().hasGC())
				return false;

			if(!FunctionGCInfoCache)
				FunctionGCInfoCache = new std::map<const Function*, GCFunctionInfo*>;

			// Visit every instruction and mark up safe points as appropriate
			for(MachineFunction::iterator basicblockiter = machinefunc.begin(); basicblockiter != machinefunc.end(); ++basicblockiter)
			{
				for(MachineBasicBlock::iterator instriter = basicblockiter->begin(); instriter != basicblockiter->end(); ++instriter)
				{
					if(instriter->isCall())
						VisitCallPoint(funcinfo, instriter, machinefunc.getTarget().getInstrInfo());
					else if(instriter->isReturn())
						VisitRet(funcinfo, instriter, machinefunc.getTarget().getInstrInfo());
				}
			}

			// Cache function information for later use by the runtime
			FunctionGCInfoCache->insert(std::make_pair(&funcinfo.getFunction(), &funcinfo));

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

				bool restart;
				do
				{
					restart = false;

					for(BasicBlock::iterator instriter = blockiter->begin(); instriter != blockiter->end(); ++instriter)
					{
						IntrinsicInst* intrinsic = dyn_cast<IntrinsicInst>(instriter);
						if(intrinsic)
						{
							Function* calltarget = intrinsic->getCalledFunction();
							switch(calltarget->getIntrinsicID())
							{
							//
							// Destroy all uses of lifetime start and end intrinsics.
							//
							// This is necessary to work around a stack coloring bug
							// which leads to GC roots being incorrectly merged into
							// other stack slots by LLVM.
							//
							// See http://llvm.org/bugs/show_bug.cgi?id=16778
							//
							case Intrinsic::lifetime_start:
							case Intrinsic::lifetime_end:
								intrinsic->eraseFromParent();

								// Must restart the loop since we just invalidated iterators
								restart = true;
								break;

							default:
								continue;
							}

							modified = true;
							if(restart)
								break;
						}
					}
				} while(restart);
			}

			// Now initialize any roots we found in this function
			for(std::vector<AllocaInst*>::iterator allocaiter = roots.begin(); allocaiter != roots.end(); ++allocaiter)
			{
				AllocaInst* allocainst = *allocaiter;
				if(allocainst->getType()->getElementType()->isPointerTy())
				{
					// Pointer types are initialized to null as usual
					StoreInst* store = new StoreInst(ConstantPointerNull::get(cast<PointerType>(cast<PointerType>(allocainst->getType())->getElementType())), *allocaiter);
					store->insertAfter(allocainst);
				}
				else if(allocainst->getType()->getElementType()->isAggregateType())
				{
					// This case handles algebraic sum types
					BitCastInst* castptr = new BitCastInst(*allocaiter, Type::getInt32PtrTy(getGlobalContext()));
					castptr->insertAfter(allocainst);
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
						// TODO - this trips a bug when trying to store reals!
						StoreInst* payloadstore = new StoreInst(ConstantInt::get(gep->getType()->getElementType(), 0), gep);
						payloadstore->insertAfter(gep);
					}
				}
				else
				{
					// This case takes care of string/buffer handles
					StoreInst* store = new StoreInst(ConstantInt::get(allocainst->getType()->getElementType(), 0), *allocaiter);
					store->insertAfter(allocainst);
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
	if(SafePointCache)
		SafePointCache->clear();
	
	if(FunctionGCInfoCache)
		FunctionGCInfoCache->clear();

	if(Globals)
		Globals->clear();
}

//
// External API for stashing the extents of a JITted function's code
//
void EpochGC::SetGCFunctionBounds(const Function* func, void*, size_t)
{
	if(!FunctionGCInfoCache)
		FunctionGCInfoCache = new std::map<const Function*, GCFunctionInfo*>;

	auto funciter = FunctionGCInfoCache->find(func);
	if(funciter == FunctionGCInfoCache->end())
		return;

	if(!SafePointCache)
		SafePointCache = new SafePointCacheT;

	GCFunctionInfo* functioninfo = funciter->second;
	for(GCFunctionInfo::iterator iter = functioninfo->begin(); iter != functioninfo->end(); ++iter)
	{
		uint64_t address = reinterpret_cast<ExecutionEngine*>(Runtime::GetThreadContext()->JITExecutionEngine)->getLabelAddress(iter->Label);
		const void* addressptr = reinterpret_cast<void*>(static_cast<unsigned>(address));
					
		SafePointCache->insert(std::make_pair(addressptr, std::make_pair(functioninfo, iter)));
	}
}

//
// External API for noting the location of global variables that might
// potentially act as stack roots.
//
void EpochGC::RegisterGlobalVariable(void* ptr, Metadata::EpochTypeID type)
{
	if(!Globals)
		Globals = new std::map<void*, Metadata::EpochTypeID>;

	Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(type);
	if(family == Metadata::EpochTypeFamily_GC || Metadata::IsStructureType(type) || family == Metadata::EpochTypeFamily_SumType)
		(*Globals)[ptr] = type;
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
		push ebp
		//call CrawlStackAndGarbageCollect
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

//
// Helper function for recording a stack bookmark
//
void GCBookmarkWorker(void* stackptr, void* retaddr)
{
	BookmarkEntry entry;
	entry.StackPointer = stackptr;
	entry.ReturnAddress = retaddr;

	if(!Bookmarks)
		Bookmarks = new std::vector<BookmarkEntry>;

	Bookmarks->push_back(entry);
}


//
// Function invoked by the runtime to add a stack bookmark
//
extern "C" __declspec(naked) void GCBookmark()
{
	__asm
	{
		// Pass the function's target return address (i.e. where we
		// will return to in our caller's code body) as well as the
		// current stack frame pointer to the bookmark helper code,
		// then clean up (since the function uses the cdecl calling
		// convention).
		push [esp]
		push ebp
		call GCBookmarkWorker
		add esp, 8

		ret
	}
}

//
// Function invoked by the runtime to clean up a stack bookmark
//
extern "C" void GCUnbookmark()
{
	Bookmarks->pop_back();
}

