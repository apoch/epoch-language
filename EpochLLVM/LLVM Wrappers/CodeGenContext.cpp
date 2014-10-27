//
// The Epoch Language Project
// Epoch Development Tools - LLVM wrapper library
//
// CODEGENCONTEXT.CPP
// Implementation of the code emission context wrapper
//


#include "Pch.h"

#include "CodeGenContext.h"
#include <sstream>


using namespace CodeGen;
using namespace llvm;


////////////////////////
class TrivialMemoryManager : public RTDyldMemoryManager {
public:
	TrivialMemoryManager(CodeGen::ThunkCallbackT funcptr, CodeGen::StringCallbackT strptr)
		: ThunkCallback(funcptr),
		  StringCallback(strptr)
	{ }

	ThunkCallbackT ThunkCallback;
	StringCallbackT StringCallback;



  SmallVector<sys::MemoryBlock, 16> FunctionMemory;
  SmallVector<sys::MemoryBlock, 16> DataMemory;

  uint8_t *allocateCodeSection(uintptr_t Size, unsigned Alignment,
                               unsigned SectionID, StringRef SectionName);
  uint8_t *allocateDataSection(uintptr_t Size, unsigned Alignment,
                               unsigned SectionID, StringRef SectionName,
                               bool IsReadOnly);

  virtual void *getPointerToNamedFunction(const std::string &Name,
                                          bool AbortOnFailure = true) {
    return 0;
  }

  bool finalizeMemory(std::string *ErrMsg) { return false; }

  // Invalidate instruction cache for sections with execute permissions.
  // Some platforms with separate data cache and instruction cache require
  // explicit cache flush, otherwise JIT code manipulations (like resolved
  // relocations) will get to the data cache but not to the instruction cache.
  virtual void invalidateInstructionCache();


	virtual uint64_t getSymbolAddress(const std::string & foo) override
	{
		if(foo.substr(0, 21) == "@epoch_static_string:")
		{
			size_t handle = 0;

			std::stringstream convert;
			convert << foo.substr(21);
			convert >> handle;

			return StringCallback(handle);
		}
		else
		{
			std::wstring wide(foo.begin(), foo.end());
			size_t offset = ThunkCallback(wide.c_str());
			return offset;
		}
	}
};

uint8_t *TrivialMemoryManager::allocateCodeSection(uintptr_t Size,
                                                   unsigned Alignment,
                                                   unsigned SectionID,
                                                   StringRef SectionName) {
  sys::MemoryBlock MB = sys::Memory::AllocateRWX(Size, 0, 0);
  FunctionMemory.push_back(MB);
  return (uint8_t*)MB.base();
}

uint8_t *TrivialMemoryManager::allocateDataSection(uintptr_t Size,
                                                   unsigned Alignment,
                                                   unsigned SectionID,
                                                   StringRef SectionName,
                                                   bool IsReadOnly) {
  sys::MemoryBlock MB = sys::Memory::AllocateRWX(Size, 0, 0);
  DataMemory.push_back(MB);
  return (uint8_t*)MB.base();
}

void TrivialMemoryManager::invalidateInstructionCache() {
  for (int i = 0, e = FunctionMemory.size(); i != e; ++i)
    sys::Memory::InvalidateInstructionCache(FunctionMemory[i].base(),
                                            FunctionMemory[i].size());

  for (int i = 0, e = DataMemory.size(); i != e; ++i)
    sys::Memory::InvalidateInstructionCache(DataMemory[i].base(),
                                            DataMemory[i].size());
}


////////////////////////



Context::Context()
	: ThunkCallback(nullptr),
	  StringCallback(nullptr)
{
	LLVMModule = new Module("EpochModule", getGlobalContext());
}


Context::~Context()
{
	delete LLVMModule;
}


void Context::SetThunkCallback(void* funcptr)
{
	ThunkCallbackT castptr = reinterpret_cast<ThunkCallbackT>(funcptr);
	ThunkCallback = castptr;
}

void Context::SetStringCallback(void* funcptr)
{
	StringCallbackT castptr = reinterpret_cast<StringCallbackT>(funcptr);
	StringCallback = castptr;
}



extern "C" void LLVMLinkInMCJIT();



size_t Context::EmitBinaryObject(char* buffer, size_t maxoutput)
{
	LLVMLinkInMCJIT();

	FunctionType* fty = FunctionType::get(Type::getVoidTy(getGlobalContext()), false);


	std::vector<Type*> paramarray;
	paramarray.push_back(Type::getInt8PtrTy(getGlobalContext())->getPointerTo());
	FunctionType* printty = FunctionType::get(Type::getVoidTy(getGlobalContext()), paramarray, false);

	
	Function* function = Function::Create(fty, GlobalValue::InternalLinkage, "entrypoint", LLVMModule);

	GlobalValue* fptr = new GlobalVariable(*LLVMModule, printty->getPointerTo(), true, GlobalValue::ExternalWeakLinkage, NULL, "ERT_print", NULL, GlobalVariable::NotThreadLocal, 0, true);
	GlobalValue* staticstrptr = new GlobalVariable(*LLVMModule, Type::getInt8PtrTy(getGlobalContext()), true, GlobalValue::ExternalWeakLinkage, NULL, "@epoch_static_string:60", NULL, GlobalVariable::NotThreadLocal, 0, true);

	IRBuilder<> builder(getGlobalContext());

	BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "entry", function);

	builder.SetInsertPoint(bb);
	Value* f = builder.CreateLoad(fptr);
	builder.CreateCall(f, staticstrptr);
	builder.CreateRetVoid();


	std::string errstr;

	TargetOptions opts;
	opts.LessPreciseFPMADOption = true;
	opts.UnsafeFPMath = true;
	opts.AllowFPOpFusion = FPOpFusion::Fast;
	opts.DisableTailCalls = false;
	opts.EnableFastISel = false;
	opts.EnableSegmentedStacks = false;
	opts.GuaranteedTailCallOpt = true;

	// Turning off frame pointer elimination can make debugging a LOT smoother...
	opts.NoFramePointerElim = true;

	TrivialMemoryManager * blobmgr = new TrivialMemoryManager(ThunkCallback, StringCallback);

	EngineBuilder eb(LLVMModule);
	//eb.setEngineKind(EngineKind::JIT);
	eb.setErrorStr(&errstr);
	//eb.setRelocationModel(Reloc::PIC_);
	//eb.setCodeModel(CodeModel::JITDefault);
	//eb.setAllocateGVsWithCode(false);
	//eb.setOptLevel(CodeGenOpt::Aggressive);
	eb.setTargetOptions(opts);
	eb.setUseMCJIT(true);
	eb.setMCJITMemoryManager(blobmgr);

	SmallVector<std::string, 2> emptyvec;
	TargetMachine* machine = eb.selectTarget(Triple("i686-pc-mingw32-elf"), "x86", "", emptyvec);
	
	ExecutionEngine* ee = eb.create(machine);
	//ExecutionEngine* ee = ExecutionEngine::MCJITCtor(module, &errstr, blobmgr, false, machine);
	if(!ee)
	{
		__asm int 3;
		return 0;
	}

	ee->DisableLazyCompilation(true);

	size_t s = 0;
	class JITListener : public llvm::JITEventListener
	{
	public:
		explicit JITListener(size_t* p)
			: P(p)
		{ }

		void NotifyObjectEmitted(const llvm::ObjectImage& img) override
		{
			llvm::error_code err;
			uint64_t baz = 0;

			for(llvm::object::symbol_iterator iter = img.begin_symbols(); iter != img.end_symbols(); iter.increment(err))
			{
				uint64_t foo = 0;
				iter->getSize(foo);
				baz += foo;
			}

			*P = (size_t)(baz);
		}

	private:
		size_t* P;
	} listen(&s);

	
	ee->RegisterJITEventListener(&listen);

	const void* p = ee->getPointerToFunction(function);
	ee->mapSectionAddress(p, 0x0404000);
	ee->finalizeObject();

	memcpy(buffer, p, s);
	return s;
}
