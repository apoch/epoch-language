//
// The Epoch Language Project
// Epoch Development Tools - LLVM wrapper library
//
// CODEGENCONTEXT.CPP
// Implementation of the code emission context wrapper
//


#include "Pch.h"

#include "CodeGenContext.h"
#include "GCCompilation.h"

#include <sstream>


using namespace CodeGen;
using namespace llvm;
using namespace Utility;



static unsigned hack = 0;


namespace
{

#include <pshpack1.h>
	struct Relocation
	{
		uint32_t address;
		uint32_t symbolindex;
		uint16_t type;
	};
#include <poppack.h>


	//
	// Manually relocate a .pdata section entry (i.e. a RUNTIME_FUNCTION structure)
	//
	// The .pdata section emitted by LLVM is given offsets from 0 instead of the correct bases.
	// Ordinarily the linker will relocate this section, but... hey guess what, we ARE the linker!
	// So instead of delegating this work, we just handle it here, since we already know the base
	// offsets of each section in the final image.
	//
	// Thankfully LLVM emits a nice set of relocation records for us so all we have to do is run
	// through the list and bump the offsets according to which section the final data should be
	// found in.
	//
	void ProcessPDataRelocationForField(const object::SectionRef& section, IMAGE_RUNTIME_FUNCTION_ENTRY* data, void* fieldaddr)
	{
		size_t fieldoffset = reinterpret_cast<const char*>(fieldaddr) - reinterpret_cast<const char*>(data);

		for(const auto& reloc : section.relocations())
		{
			if(reloc.getOffset() == fieldoffset)
			{
				if(reloc.getSymbol()->getName().get().str() == ".xdata")
				{
					*reinterpret_cast<char**>(fieldaddr) += 0x4000;		// TODO - don't hardcode the offset of the .xdata section
				}
				else
				{
					*reinterpret_cast<char**>(fieldaddr) += reloc.getSymbol()->getAddress().get() + 0x8000;    // TODO - don't hardcode the offset of the .text section
				}
			}
		}
	}

	//
	// Batch relocate all .pdata RUNTIME_FUNCTION structures to the correct offsets
	//
	// See comments on ProcessPDataRelocationForField for details on why this is necessary.
	//
	// This could be rewritten to be a little messier but a lot faster. Until profiling indicates
	// that link time/image emission time is significantly hampered by this process, however, it
	// isn't worth the effort or the loss of clarity.
	//
	void ProcessPDataRelocations(const object::SectionRef& section, std::vector<char>* buffer)
	{
		size_t numrecords = buffer->size() / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
		IMAGE_RUNTIME_FUNCTION_ENTRY* data = reinterpret_cast<IMAGE_RUNTIME_FUNCTION_ENTRY*>(buffer->data());

		for(size_t i = 0; i < numrecords; ++i)
		{
			ProcessPDataRelocationForField(section, data, &(data[i].BeginAddress));
			ProcessPDataRelocationForField(section, data, &(data[i].EndAddress));
			ProcessPDataRelocationForField(section, data, &(data[i].UnwindInfoAddress));
		}
	}


	void ProcessArbitraryRelocations(const object::SectionRef& section, std::vector<char>* buffer)
	{
		for(const auto& reloc : section.relocations())
		{
			unsigned idx = 0;
			for(const auto& sym : section.getObject()->symbols())
			{
				if(sym.getName().get() == reloc.getSymbol()->getName().get())
					break;
				
				++idx;
			}

			Relocation relocStruct;
			relocStruct.type = static_cast<uint16_t>(reloc.getType());
			relocStruct.address = static_cast<uint32_t>(reloc.getOffset());
			relocStruct.symbolindex = static_cast<uint32_t>(idx);

			AppendToBuffer(buffer, relocStruct);
		}
	}


	//
	// Memory management wrapper for handling image emission/linking
	//
	// Conforms to LLVM's interfaces for this sort of management in a minimalistic way.
	// Mostly we use it for injecting the addresses of our own symbols in a way that is
	// somewhat decoupled from LLVM's symbol management infrastructure. However there's
	// also a convenient hook here for grabbing the actual memory address at which LLVM
	// emits code, so we can relocate it correctly later.
	//
	class TrivialMemoryManager : public RTDyldMemoryManager
	{
	public:
		TrivialMemoryManager(CodeGen::ThunkCallbackT funcptr, CodeGen::StringCallbackT strptr, uint64_t* outAddr, size_t* outSize, uint64_t* outPData, uint64_t* outXData)
			: ThunkCallback(funcptr),
			  StringCallback(strptr),
			  OutAddr(outAddr),
			  OutSize(outSize),
			  OutPDataOffset(outPData),
			  OutXDataOffset(outXData)
		{ }

		uint8_t* allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName) override;
		uint8_t* allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName, bool IsReadOnly) override;

		bool finalizeMemory(std::string*) override
		{
			return false;
		}

		void* getPointerToNamedFunction(const std::string& Name, bool AbortOnFailure = true) override
		{
			return nullptr;
		}

		uint64_t getSymbolAddress(const std::string& foo) override;

	private:		// Internal state
		ThunkCallbackT ThunkCallback;
		StringCallbackT StringCallback;

		uint64_t* OutAddr;
		size_t* OutSize;

		uint64_t* OutPDataOffset;
		uint64_t* OutXDataOffset;

		SmallVector<sys::MemoryBlock, 16> FunctionMemory;
		SmallVector<sys::MemoryBlock, 16> DataMemory;
	};

	uint8_t* TrivialMemoryManager::allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName)
	{
		sys::MemoryBlock MB = sys::Memory::AllocateRWX(Size, 0, 0);

		*OutAddr = (uint64_t)MB.base();
		*OutSize = Size;

		FunctionMemory.push_back(MB);
		return (uint8_t*)MB.base();
	}

	uint8_t* TrivialMemoryManager::allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName, bool IsReadOnly)
	{
		sys::MemoryBlock MB = sys::Memory::AllocateRWX(Size, 0, 0);

		if(SectionName == ".pdata")
			*OutPDataOffset = (uint64_t)MB.base();
		else if(SectionName == ".xdata")
			*OutXDataOffset = (uint64_t)MB.base();

		DataMemory.push_back(MB);
		return (uint8_t*)MB.base();
	}

	//
	// Resolve a symbol to a concrete address.
	//
	// We support two kinds of symbol resolution: static strings, and thunk functions.
	// Static strings are magically identified by a prefix token. They are mapped by a
	// lookup table in the Epoch compiler itself, and we simply ask the compiler for a
	// concrete address for each string. Thunk functions are the same basic setup, but
	// without a name prefix token. Therefore, any symbol that isn't a string is going
	// to be resolved as a thunk.
	//
	uint64_t TrivialMemoryManager::getSymbolAddress(const std::string& foo)
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

			assert(offset != 0);

			return offset;
		}
	}


}



Context::Context()
	: ThunkCallback(nullptr),
	  StringCallback(nullptr),
	  LLVMBuilder(getGlobalContext()),
	  EntryPointFunction(nullptr),
	  LLVMModule(std::make_unique<Module>("EpochModule", getGlobalContext())),
	  DebugBuilder(*LLVMModule)
{
	LLVMModule->addModuleFlag(Module::ModFlagBehavior::Warning, "CodeView", 1);

	// TODO - stash CUs for each file of the input program; will require debug info internally in EpochCompiler
	// TODO - change params to handle optimized code when optimizations are back in
	DebugCompileUnit = DebugBuilder.createCompileUnit(dwarf::DW_LANG_C, "LinkedProgram.epoch", "D:\\epoch\\epoch-language\\x64\\debug", "Epoch Compiler", false, "", 0);
	DebugFile = DebugBuilder.createFile(DebugCompileUnit->getFilename(), DebugCompileUnit->getDirectory());

	FunctionType* initfunctiontype = FunctionType::get(Type::getVoidTy(getGlobalContext()), false);
	InitFunction = Function::Create(initfunctiontype, GlobalValue::ExternalLinkage, "init", LLVMModule.get());
	InitFunction->setGC("EpochGC");

	SetupDebugInfo(InitFunction);

	{
		std::vector<Type*> args;
		args.push_back(Type::getInt8PtrTy(getGlobalContext())->getPointerTo());
		args.push_back(Type::getInt8PtrTy(getGlobalContext()));
		FunctionType* ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), args, false);
		GCRootFunction = Function::Create(ftype, Function::ExternalLinkage, "llvm.gcroot", LLVMModule.get());
	}
}


Context::~Context()
{
	// TODO - clean up ExecutionEngine and any other auxiliary LLVM structures
}


void Context::FunctionQueueParamType(llvm::Type* ty)
{
	PendingParamTypeStack.back().push_back(ty);
}


llvm::FunctionType* Context::FunctionTypeCreate(llvm::Type* rettype)
{
	llvm::FunctionType* fty = FunctionType::get(rettype, PendingParamTypeStack.back(), false);
	PendingParamTypeStack.pop_back();

	return fty;
}

void Context::FunctionTypePush()
{
	PendingParamTypeStack.push_back(std::vector<llvm::Type*>());
}

llvm::Function* Context::FunctionCreate(const char* name, llvm::FunctionType* fty)
{
	Function* func = Function::Create(fty, GlobalValue::ExternalLinkage, name, LLVMModule.get());
	func->setGC("EpochGC");
	SetupDebugInfo(func);
	return func;
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
	//assert(PendingValues.empty());
}


void Context::SetEntryFunction(llvm::Function* entryfunc)
{
	EntryPointFunction = entryfunc;
}


void Context::SetThunkCallback(void* funcptr)
{
	ThunkCallback = reinterpret_cast<ThunkCallbackT>(funcptr);
}

void Context::SetStringCallback(void* funcptr)
{
	StringCallback = reinterpret_cast<StringCallbackT>(funcptr);
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

llvm::Type* Context::TypeGetInteger64()
{
	return Type::getInt64Ty(getGlobalContext());
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

llvm::Type* Context::TypeGetBuffer()
{
	return Type::getInt8PtrTy(getGlobalContext());
}



extern "C" void LLVMLinkInMCJIT();



size_t Context::EmitBinaryObject(char* buffer, size_t maxoutput)
{
	LLVMLinkInMCJIT();

	std::vector<Type*> argtypes;
	argtypes.push_back(TypeGetInteger());

	FunctionType* voidtype = FunctionType::get(TypeGetVoid(), false);
	FunctionType* exitprocesstype = FunctionType::get(TypeGetVoid(), argtypes, false);

	GlobalVariable* exitprocessfunctionvar = FunctionCreateThunk("ExitProcess", exitprocesstype);
	GlobalVariable* gcinitfunctionvar = FunctionCreateThunk("ERT_gc_init", exitprocesstype);
	GlobalVariable* gccollectstrsfunctionvar = FunctionCreateThunk("ERT_gc_collect_strings", voidtype);

	
	BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "InitBlock", InitFunction);
	LLVMBuilder.SetInsertPoint(bb);
	LLVMBuilder.CreateCall(LLVMBuilder.CreateLoad(gcinitfunctionvar), ConstantInt::get(TypeGetInteger(), 0x6000));		// TODO - un-hardcode .gc segment address
	TagDebugLine(++hack, 0);

	// TODO - init globals here
	LLVMBuilder.CreateCall(EntryPointFunction);
	TagDebugLine(++hack, 0);

	LLVMBuilder.CreateCall(LLVMBuilder.CreateLoad(gccollectstrsfunctionvar));
	TagDebugLine(++hack, 0);

	LLVMBuilder.CreateCall(LLVMBuilder.CreateLoad(exitprocessfunctionvar), ConstantInt::get(TypeGetInteger(), 0));
	TagDebugLine(++hack, 0);

	LLVMBuilder.CreateRetVoid();
	//LLVMBuilder.CreateRet(ConstantInt::get(TypeGetInteger(), 0));
	TagDebugLine(++hack, 0);

	DebugBuilder.finalize();

	llvm::raw_os_ostream spew(std::cout);
	if(llvm::verifyModule(*LLVMModule, &spew))
	{
		spew.flush();

		LLVMModule->dump();
		exit(666);
	}
	

	std::string errstr;

	TargetOptions opts;
	opts.LessPreciseFPMADOption = true;
	opts.UnsafeFPMath = true;
	opts.AllowFPOpFusion = FPOpFusion::Fast;
	opts.EnableFastISel = false;
	opts.GuaranteedTailCallOpt = true;

	uint64_t pdata = 0;
	uint64_t xdata = 0;
	uint64_t emissionaddr = 0;
	size_t s = 0;
	std::unique_ptr<TrivialMemoryManager> blobmgr = std::make_unique<TrivialMemoryManager>(ThunkCallback, StringCallback, &emissionaddr, &s, &pdata, &xdata);

	// HACK! We move the smart pointer's contents into the EngineBuilder
	// below, but we still want to access the module for other purposes.
	Module* llvmmodule = LLVMModule.get();


	EngineBuilder eb(std::move(LLVMModule));
	eb.setErrorStr(&errstr);
	eb.setTargetOptions(opts);
	eb.setMCJITMemoryManager(std::move(blobmgr));

	SmallVector<std::string, 2> emptyvec;
	TargetMachine* machine = eb.selectTarget(Triple("x86_64-pc-windows-msvc"), "", "", emptyvec);
	
	ExecutionEngine* ee = eb.create(machine);
	if(!ee)
	{
		return 0;
	}

	llvmmodule->setDataLayout(ee->getDataLayout());

	// TODO - reintroduce optimizations

	llvmmodule->dump();


	const object::ObjectFile* image;

	class JEL : public JITEventListener
	{
		uint64_t* OutEmissionAddr;
		size_t* OutSize;

		const object::ObjectFile** OutImage;

	public:
		JEL(uint64_t* outaddr, size_t* outsize, const object::ObjectFile** image)
			: OutSize(outsize),
			  OutEmissionAddr(outaddr),
			  OutImage(image)
		{ }

		void NotifyObjectEmitted(const object::ObjectFile& img, const RuntimeDyld::LoadedObjectInfo& info) override
		{
			*OutSize = 0;

			*OutImage = &img;

			for(const auto & section : img.sections())
			{
				if(section.isText())
				{
					*OutSize += static_cast<size_t>(section.getSize());
				}
			}
		}
	} listener(&emissionaddr, &s, &image);

	ee->RegisterJITEventListener(&listener);

	ee->DisableLazyCompilation(true);
	ee->generateCodeForModule(llvmmodule);
	ee->mapSectionAddress((void*)(emissionaddr), 0x0408000);				// TODO
	ee->finalizeObject();


	for(const auto& section : image->sections())
	{
		if(!section.isText() && !section.isBSS() && !section.isVirtual())
		{	
			std::vector<char>* targetbuffer = nullptr;

			StringRef sectionname;
			section.getName(sectionname);

			if(sectionname == ".pdata")
				targetbuffer = &PData;
			else if(sectionname == ".xdata")
				targetbuffer = &XData;
			else if(sectionname == ".debug$S")
				targetbuffer = &DebugData;

			if(targetbuffer)
			{
				StringRef sectiondata;
				section.getContents(sectiondata);
				std::copy(sectiondata.begin(), sectiondata.end(), std::back_inserter(*targetbuffer));
			}

			if(sectionname == ".pdata")
				ProcessPDataRelocations(section, targetbuffer);
			else if(sectionname == ".debug$S")
				ProcessArbitraryRelocations(section, &DebugRelocs);
		}
	}



	std::vector<char> stringbuffer;
	unsigned numsyms = 0;

	uint32_t offset = 4;
	for(const auto& sym : image->symbols())
	{
		IMAGE_SYMBOL symbol;

		memset(symbol.N.ShortName, 0, 8);

		auto nameerr = sym.getName();
		if(!nameerr)
			continue;

		auto nameref = nameerr.get();
		auto symname = nameref.str();

		symbol.N.LongName[1] = offset;


		std::copy(std::begin(symname), std::end(symname), std::back_inserter(stringbuffer));
		stringbuffer.push_back(0);
		offset += symname.length() + 1;

		symbol.Value = (DWORD)sym.getValue();
		symbol.SectionNumber = IMAGE_SYM_ABSOLUTE;
		symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
		
		switch(sym.getType())
		{
		case object::SymbolRef::ST_Function:
			symbol.SectionNumber = 8;

			if(symname == "init")
				symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;

			symbol.Type = (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
			break;

		default:
			symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
			symbol.SectionNumber = IMAGE_SYM_ABSOLUTE;
			symbol.Value = 0;
			symbol.Type = (IMAGE_SYM_DTYPE_POINTER << N_BTSHFT);
			break;
		}
		
		symbol.NumberOfAuxSymbols = 0;

		AppendToBuffer<IMAGE_SYMBOL, IMAGE_SIZEOF_SYMBOL>(&DebugSymbols, symbol);
		++numsyms;
	}

	AppendToBuffer(&DebugSymbols, uint32_t(stringbuffer.size() + 8));

	std::copy(std::begin(stringbuffer), std::end(stringbuffer), std::back_inserter(DebugSymbols));


	GCCompilation::PrepareGCData(*ee, &GCSection);


	memcpy(buffer, (void*)(emissionaddr), s);
	return s;
}


llvm::AllocaInst* Context::CodeCreateAlloca(llvm::Type* vartype, const char* varname)
{
	auto allocainst = LLVMBuilder.CreateAlloca(vartype, nullptr, varname);

	if(vartype == Type::getInt8PtrTy(getGlobalContext()))
	{
		Value* signature = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0x02000000);
		Value* constant = LLVMBuilder.CreateIntToPtr(signature, Type::getInt8PtrTy(getGlobalContext()));
		Value* castptr = LLVMBuilder.CreatePointerCast(allocainst, Type::getInt8PtrTy(getGlobalContext())->getPointerTo());
		LLVMBuilder.CreateCall(GCRootFunction, { castptr, constant });
	}

	return allocainst;
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

void Context::CodeCreateCondBranch(Value* cond, BasicBlock* truetarget, BasicBlock* falsetarget)
{
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

void Context::CodeCreateOperatorIntegerNotEquals()
{
	llvm::Value* operand2 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* operand1 = PendingValues.back();
	PendingValues.pop_back();

	llvm::Value* eqval = LLVMBuilder.CreateICmpNE(operand1, operand2);
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

void Context::CodePushInteger16(short value)
{
	llvm::Value* val = ConstantInt::get(Type::getInt16Ty(getGlobalContext()), value);
	PendingValues.push_back(val);
}

void Context::CodePushInteger64(uint64_t value)
{
	llvm::Value* val = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value);
	PendingValues.push_back(val);
}

void Context::CodePushReal(float value)
{
	llvm::Value* val = ConstantFP::get(Type::getFloatTy(getGlobalContext()), value);
	PendingValues.push_back(val);
}

void Context::CodePushRawAlloca(llvm::AllocaInst* alloc)
{
	PendingValues.push_back(alloc);
}

void Context::CodePushRawCall(llvm::CallInst* callinst)
{
	PendingValues.push_back(callinst);
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
	TagDebugLine(++hack, 0);

	assert(PendingValues.empty() || PendingValues.size() == 1);
	PendingValues.clear();
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
	llvm::Type* t = llvm::StructType::create(PendingMemberTypes, name);
	PendingMemberTypes.clear();

	return t;
}

void Context::StructureTypeQueueMember(llvm::Type* t)
{
	PendingMemberTypes.push_back(t);
}



void Context::SectionCopyPData(void* buffer) const
{
	memcpy(buffer, PData.data(), PData.size());
}

void Context::SectionCopyXData(void* buffer) const
{
	memcpy(buffer, XData.data(), XData.size());
}

void Context::SectionCopyGC(void* buffer) const
{
	memcpy(buffer, GCSection.data(), GCSection.size());
}

void Context::SectionCopyDebug(void* buffer) const
{
	memcpy(buffer, DebugData.data(), DebugData.size());
}

void Context::SectionCopyDebugReloc(void* buffer) const
{
	memcpy(buffer, DebugRelocs.data(), DebugRelocs.size());
}

void Context::SectionCopyDebugSymbols(void* buffer) const
{
	memcpy(buffer, DebugSymbols.data(), DebugSymbols.size());
}



unsigned Context::SectionGetPDataSize() const
{
	return static_cast<unsigned>(PData.size());
}

unsigned Context::SectionGetXDataSize() const
{
	return static_cast<unsigned>(XData.size());
}

unsigned Context::SectionGetGCSize() const
{
	return static_cast<unsigned>(GCSection.size());
}

unsigned Context::SectionGetDebugSize() const
{
	return static_cast<unsigned>(DebugData.size());
}

unsigned Context::SectionGetDebugRelocSize() const
{
	return static_cast<unsigned>(DebugRelocs.size());
}

unsigned Context::SectionGetDebugSymbolSize() const
{
	return static_cast<unsigned>(DebugSymbols.size());
}



llvm::Value* Context::CodePopValue()
{
	Value* v = PendingValues.back();
	PendingValues.pop_back();
	return v;
}



void Context::SetupDebugInfo(Function* function)
{
	DIScope* fcontext = DebugCompileUnit;
	unsigned line = 0;
	unsigned scopeline = 0;

	std::vector<Metadata*> argtypes;
	argtypes.push_back(TypeGetDebugType(function->getReturnType()));

	for(auto& arg : function->getArgumentList())
		argtypes.push_back(TypeGetDebugType(arg.getType()));

	DISubroutineType* debugtype = DebugBuilder.createSubroutineType(DebugBuilder.getOrCreateTypeArray(argtypes));

	DISubprogram* subprogram = DebugBuilder.createFunction(fcontext, function->getName(), StringRef(), DebugFile, line, debugtype, false, true, scopeline, DINode::FlagPrototyped, false);

	/*
	unsigned i = 1;
	for(auto& arg : function->getArgumentList())
	{
		DebugBuilder.createParameterVariable(subprogram, arg.getName(), i, DebugFile, 0, (DIType*)(argtypes[i]), false, 0);
		++i;
	}
	*/
	function->setSubprogram(subprogram);
}


llvm::DIType* Context::TypeGetDebugType(Type* t)
{
	// TODO - build better type data
	return DebugBuilder.createBasicType("placeholder", 32, 32, dwarf::DW_ATE_unsigned);
}

void Context::TagDebugLine(unsigned line, unsigned column)
{
	DebugLoc loc = DILocation::get(getGlobalContext(), line, column, LLVMBuilder.GetInsertBlock()->getParent()->getSubprogram());
	LLVMBuilder.GetInsertBlock()->getInstList().back().setDebugLoc(loc);
}


llvm::Type* Context::SumTypeCreate(const char* name, unsigned width)
{
	std::vector<llvm::Type*> members;
	members.push_back(llvm::Type::getInt32Ty(getGlobalContext()));
	members.push_back(llvm::Type::getIntNTy(getGlobalContext(), width * 8));

	llvm::Type* t = llvm::StructType::create(members, name);
	return t;
}


