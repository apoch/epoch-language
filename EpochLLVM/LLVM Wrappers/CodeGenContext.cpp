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
	TrivialMemoryManager(CodeGen::ThunkCallbackT funcptr, CodeGen::StringCallbackT strptr, uint64_t* outAddr, size_t* outSize)
		: ThunkCallback(funcptr),
		  StringCallback(strptr),
		  OutAddr(outAddr),
		  OutSize(outSize)
	{ }

	ThunkCallbackT ThunkCallback;
	StringCallbackT StringCallback;

	uint64_t* OutAddr;
	size_t* OutSize;



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

  *OutAddr = (uint64_t)MB.base();
  *OutSize = Size;

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
	  StringCallback(nullptr),
	  LLVMBuilder(getGlobalContext()),
	  EntryPointFunction(nullptr),
	  LLVMModule(new Module("EpochModule", getGlobalContext()))
{
	FunctionType* initfunctiontype = FunctionType::get(Type::getInt32Ty(getGlobalContext()), false);
	InitFunction = Function::Create(initfunctiontype, GlobalValue::ExternalLinkage , "@init", LLVMModule.get());
}


Context::~Context()
{
}


void Context::FunctionQueueParamType(llvm::Type* ty)
{
	PendingParamTypes.push_back(ty);
}


llvm::FunctionType* Context::FunctionTypeCreate(llvm::Type* rettype)
{
	llvm::FunctionType* fty = FunctionType::get(rettype, PendingParamTypes, false);
	PendingParamTypes.clear();

	return fty;
}

llvm::Function* Context::FunctionCreate(const char* name, llvm::FunctionType* fty)
{
	return Function::Create(fty, GlobalValue::ExternalLinkage, name, LLVMModule.get());
}

llvm::GlobalVariable* Context::FunctionCreateThunk(const char* name, llvm::FunctionType* fty)
{
	llvm::GlobalVariable* var = CachedThunkFunctions[name];
	if(!var)
	{
		var = new GlobalVariable(*LLVMModule, fty->getPointerTo(), true, GlobalValue::ExternalWeakLinkage, NULL, name, NULL, GlobalVariable::NotThreadLocal, 0, true);
		CachedThunkFunctions[name] = var;
	}

	return var;
}

void Context::FunctionFinalize()
{
	//LLVMBuilder.GetInsertBlock()->getParent()->dump();

	// TODO - better implementation of this
	//if(!PendingValues.empty())
	//{
	//	exit(666);
	//}
}


void Context::SetEntryFunction(llvm::Function* entryfunc)
{
	EntryPointFunction = entryfunc;
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


llvm::Type* Context::TypeGetBoolean()
{
	return Type::getInt1Ty(getGlobalContext());
}

llvm::Type* Context::TypeGetInteger()
{
	return Type::getInt32Ty(getGlobalContext());
}

llvm::Type* Context::TypeGetInteger16()
{
	return Type::getInt16Ty(getGlobalContext());
}

llvm::Type* Context::TypeGetPointerTo(llvm::Type* raw)
{
	return raw->getPointerTo();
}

llvm::Type* Context::TypeGetReal()
{
	return Type::getFloatTy(getGlobalContext());
}

llvm::Type* Context::TypeGetString()
{
	return Type::getInt8PtrTy(getGlobalContext());
}

llvm::Type* Context::TypeGetVoid()
{
	return Type::getVoidTy(getGlobalContext());
}



extern "C" void LLVMLinkInMCJIT();



size_t Context::EmitBinaryObject(char* buffer, size_t maxoutput)
{
	LLVMLinkInMCJIT();
	
	BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "InitBlock", InitFunction);
	LLVMBuilder.SetInsertPoint(bb);
	// TODO - init globals here
	LLVMBuilder.CreateCall(EntryPointFunction);
	LLVMBuilder.CreateRet(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0));

	// HACK!
	Module* wat = LLVMModule.get();

	/*
	llvm::raw_os_ostream spew(std::cout);
	if(!llvm::verifyModule(*LLVMModule, &spew))
	{
		spew.flush();

		wat->dump();
		exit(666);
	}
	*/

	std::string errstr;

	TargetOptions opts;
	opts.LessPreciseFPMADOption = true;
	opts.UnsafeFPMath = true;
	opts.AllowFPOpFusion = FPOpFusion::Fast;
	//opts.DisableTailCalls = false;
	opts.EnableFastISel = false;
	//opts.EnableSegmentedStacks = false;
	opts.GuaranteedTailCallOpt = true;
	//opts.NoFramePointerElim = true;			// TODO - uhhhhh

	uint64_t emissionaddr = 0;
	size_t s = 0;
	std::unique_ptr<TrivialMemoryManager> blobmgr = std::make_unique<TrivialMemoryManager>(ThunkCallback, StringCallback, &emissionaddr, &s);

	EngineBuilder eb(std::move(LLVMModule));
	eb.setErrorStr(&errstr);
	eb.setTargetOptions(opts);
	//eb.setUseMCJIT(true);
	//eb.setRelocationModel(Reloc::Static);
	eb.setMCJITMemoryManager(std::move(blobmgr));

	SmallVector<std::string, 2> emptyvec;
	TargetMachine* machine = eb.selectTarget(Triple("x86_64-pc-windows-elf"), "", "", emptyvec);
	
	ExecutionEngine* ee = eb.create(machine);
	if(!ee)
	{
		return 0;
	}





	legacy::FunctionPassManager fpm(wat);
	wat->setDataLayout(ee->getDataLayout());
	//fpm.add(createTypeBasedAliasAnalysisPass());
	//fpm.add(createBasicAliasAnalysisPass());
	fpm.add(createCFGSimplificationPass());
	fpm.add(createScalarReplAggregatesPass());
	fpm.add(createEarlyCSEPass());
	fpm.add(createLowerExpectIntrinsicPass());

	fpm.doInitialization();
	
	legacy::PassManager mpm;
	//mpm.add(createTypeBasedAliasAnalysisPass());
	//mpm.add(createBasicAliasAnalysisPass());
	mpm.add(createGlobalOptimizerPass());
	mpm.add(createPromoteMemoryToRegisterPass());
	mpm.add(createIPSCCPPass());
	mpm.add(createDeadArgEliminationPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createPruneEHPass());
	//mpm.add(createFunctionAttrsPass());
	mpm.add(createFunctionInliningPass());
	mpm.add(createArgumentPromotionPass());
	mpm.add(createScalarReplAggregatesPass(-1, false));
	mpm.add(createEarlyCSEPass());
	mpm.add(createJumpThreadingPass());
	mpm.add(createCorrelatedValuePropagationPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createTailCallEliminationPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createReassociatePass());
	mpm.add(createLoopRotatePass());
	mpm.add(createLICMPass());
	mpm.add(createLoopUnswitchPass(false));
	mpm.add(createInstructionCombiningPass());
	mpm.add(createIndVarSimplifyPass());
	mpm.add(createLoopIdiomPass());
	mpm.add(createLoopDeletionPass());
	mpm.add(createLoopUnrollPass());
	mpm.add(createGVNPass());
	mpm.add(createMemCpyOptPass());
	mpm.add(createSCCPPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createJumpThreadingPass());
	mpm.add(createCorrelatedValuePropagationPass());
	mpm.add(createDeadStoreEliminationPass());
	mpm.add(createAggressiveDCEPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createFunctionInliningPass());
	mpm.add(createDeadStoreEliminationPass());

	mpm.run(*wat);


	//wat->dump();


	class JEL : public JITEventListener
	{
		uint64_t* OutEmissionAddr;
		size_t* OutSize;

	public:
		JEL(uint64_t* outaddr, size_t* outsize)
			: OutSize(outsize),
			  OutEmissionAddr(outaddr)
		{ }

		void NotifyObjectEmitted(const object::ObjectFile& img, const RuntimeDyld::LoadedObjectInfo& info) override
		{
			*OutSize = 0;

			for(const auto & section : img.sections())
			{
				if(section.isText())
				{
					*OutSize += static_cast<size_t>(section.getSize());
				}
			}
		}
	} listener(&emissionaddr, &s);

	ee->RegisterJITEventListener(&listener);

	ee->DisableLazyCompilation(true);
	ee->generateCodeForModule(wat);
	ee->mapSectionAddress((void*)(emissionaddr), 0x0404000);
	ee->finalizeObject();

	memcpy(buffer, (void*)(emissionaddr), s);
	return s;
}


llvm::AllocaInst* Context::CodeCreateAlloca(llvm::Type* vartype, const char* varname)
{
	return LLVMBuilder.CreateAlloca(vartype, nullptr, varname);
}

llvm::BasicBlock* Context::CodeCreateBasicBlock(llvm::Function* parent, bool setinsertpoint)
{
	BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "", parent);
	if(setinsertpoint)
		LLVMBuilder.SetInsertPoint(bb);
	return bb;
}

void Context::CodeCreateBranch(BasicBlock* target, bool setinsertpoint)
{
	LLVMBuilder.CreateBr(target);
	if(setinsertpoint)
		LLVMBuilder.SetInsertPoint(target);
}

llvm::CallInst* Context::CodeCreateCall(llvm::Function* target)
{
	llvm::FunctionType* fty = target->getFunctionType();

	std::vector<Value*> relevantargs;
	for(size_t i = 0; i < fty->getNumParams(); ++i)
	{
		relevantargs.push_back(PendingValues.back());
		PendingValues.pop_back();
	}
	std::reverse(relevantargs.begin(), relevantargs.end());
	
	llvm::CallInst* inst = LLVMBuilder.CreateCall(target, relevantargs);

	if(inst->getType() != Type::getVoidTy(getGlobalContext()))
		PendingValues.push_back(inst);

	return inst;
}

void Context::CodeCreateCallIndirect(llvm::AllocaInst* targetAlloca)
{
	llvm::Value* target = LLVMBuilder.CreateLoad(targetAlloca);
	llvm::FunctionType* fty = cast<llvm::FunctionType>(cast<llvm::PointerType>(target->getType())->getElementType());

	std::vector<Value*> relevantargs;
	for(size_t i = 0; i < fty->getNumParams(); ++i)
	{
		relevantargs.push_back(PendingValues.back());
		PendingValues.pop_back();
	}
	std::reverse(relevantargs.begin(), relevantargs.end());

	llvm::CallInst* inst = LLVMBuilder.CreateCall(target, relevantargs);

	if(inst->getType() != Type::getVoidTy(getGlobalContext()))
		PendingValues.push_back(inst);
}

llvm::CallInst* Context::CodeCreateCallThunk(llvm::GlobalVariable* target)
{
	llvm::Value* loadedTarget = LLVMBuilder.CreateLoad(target);
	llvm::FunctionType* fty = llvm::cast<llvm::FunctionType>(loadedTarget->getType()->getContainedType(0));

	std::vector<Value*> relevantargs;
	for(size_t i = 0; i < fty->getNumParams(); ++i)
	{
		relevantargs.push_back(PendingValues.back());
		PendingValues.pop_back();
	}
	std::reverse(relevantargs.begin(), relevantargs.end());

	llvm::CallInst* inst = LLVMBuilder.CreateCall(loadedTarget, relevantargs);

	if(inst->getType() != Type::getVoidTy(getGlobalContext()))
		PendingValues.push_back(inst);

	return inst;
}

void Context::CodeCreateCondBranch(BasicBlock* truetarget, BasicBlock* falsetarget)
{
	Value* cond = PendingValues.back();
	PendingValues.pop_back();

	LLVMBuilder.CreateCondBr(cond, truetarget, falsetarget);
}

void Context::CodeCreateDereference()
{
	Value* p = PendingValues.back();
	PendingValues.pop_back();

	Value* l = LLVMBuilder.CreateLoad(p);
	PendingValues.push_back(l);
}

llvm::Value* Context::CodeCreateGEP(unsigned index)
{
	Value* v = PendingValues.back();
	PendingValues.pop_back();

	Value* indices[] = {ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0), ConstantInt::get(Type::getInt32Ty(getGlobalContext()), index)};

	return LLVMBuilder.CreateGEP(v, indices);
}

void Context::CodeCreateRead(llvm::AllocaInst* allocatarget)
{
	Value* rv = LLVMBuilder.CreateLoad(allocatarget);
	PendingValues.push_back(rv);
}

void Context::CodeCreateReadParam(unsigned index)
{
	auto iter = LLVMBuilder.GetInsertBlock()->getParent()->arg_begin();
	std::advance(iter, index);

	Value* rv = static_cast<Argument*>(iter);
	PendingValues.push_back(rv);
}

void Context::CodeCreateReadStructure(llvm::Value* gep)
{
	Value* rv = LLVMBuilder.CreateLoad(gep);
	PendingValues.push_back(rv);
}

void Context::CodeCreateRet()
{
	llvm::Value* retval = PendingValues.back();
	PendingValues.pop_back();

	LLVMBuilder.CreateRet(retval);
}

void Context::CodeCreateRetVoid()
{
	LLVMBuilder.CreateRetVoid();
}

void Context::CodeCreateWrite(llvm::AllocaInst* allocatarget)
{
	Value* wv = PendingValues.back();
	PendingValues.pop_back();

	LLVMBuilder.CreateStore(wv, allocatarget);
}

void Context::CodeCreateWriteParam(unsigned index)
{
	auto iter = LLVMBuilder.GetInsertBlock()->getParent()->arg_begin();
	std::advance(iter, index);

	Value* pv = static_cast<Argument*>(iter);

	Value* wv = PendingValues.back();
	PendingValues.pop_back();

	LLVMBuilder.CreateStore(wv, pv);
}

void Context::CodeCreateWriteStructure(llvm::Value* gep)
{
	Value* wv = PendingValues.back();
	PendingValues.pop_back();

	LLVMBuilder.CreateStore(wv, gep);
}

void Context::CodeCreateWriteStructurePop()
{
	Value* gep = PendingValues.back();
	PendingValues.pop_back();

	Value* wv = PendingValues.back();
	PendingValues.pop_back();

	LLVMBuilder.CreateStore(wv, gep);
}

void Context::CodeCreateOperatorBooleanNot()
{
	llvm::Value* val = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* notval = LLVMBuilder.CreateNot(val);
	PendingValues.push_back(notval);
}

void Context::CodeCreateOperatorIntegerEquals()
{
	llvm::Value* operand2 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* operand1 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* eqval = LLVMBuilder.CreateICmpEQ(operand1, operand2);
	PendingValues.push_back(eqval);
}

void Context::CodeCreateOperatorIntegerPlus()
{
	llvm::Value* operand2 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* operand1 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* val = LLVMBuilder.CreateAdd(operand1, operand2);
	PendingValues.push_back(val);
}

void Context::CodeCreateOperatorIntegerMinus()
{
	llvm::Value* operand2 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* operand1 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* val = LLVMBuilder.CreateSub(operand1, operand2);
	PendingValues.push_back(val);
}


void Context::CodePushBoolean(bool value)
{
	llvm::Value* val = ConstantInt::get(Type::getInt1Ty(getGlobalContext()), value);
	PendingValues.push_back(val);
}

void Context::CodePushInteger(int value)
{
	llvm::Value* val = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), value);
	PendingValues.push_back(val);
}

void Context::CodePushRawAlloca(llvm::AllocaInst* alloc)
{
	PendingValues.push_back(alloc);
}

void Context::CodePushRawGEP(llvm::Value* gep)
{
	PendingValues.push_back(gep);
}

void Context::CodePushString(unsigned handle)
{
	llvm::GlobalVariable* val = CachedStrings[handle];
	if(!val)
	{
		std::ostringstream name;
		name << "@epoch_static_string:" << handle;
		val = new GlobalVariable(*LLVMModule, Type::getInt8Ty(getGlobalContext()), true, GlobalValue::ExternalWeakLinkage, NULL, name.str(), NULL, GlobalVariable::NotThreadLocal, 0, true);

		CachedStrings[handle] = val;
	}

	PendingValues.push_back(val);
}

void Context::CodePushFunction(llvm::Function* func)
{
	PendingValues.push_back(func);
}


void Context::CodeStatementFinalize()
{
	//PendingValues.clear();
}


llvm::BasicBlock* Context::GetCurrentBasicBlock()
{
	return LLVMBuilder.GetInsertBlock();
}

void Context::SetCurrentBasicBlock(llvm::BasicBlock* block)
{
	LLVMBuilder.SetInsertPoint(block);
}


llvm::Type* Context::StructureTypeCreate(const char* name)
{
	llvm::Type* t = llvm::StructType::create(PendingParamTypes, name);
	PendingParamTypes.clear();
	return t;
}

void Context::StructureTypeQueueMember(llvm::Type* t)
{
	PendingParamTypes.push_back(t);
}

