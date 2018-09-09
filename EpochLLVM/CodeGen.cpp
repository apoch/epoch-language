#include "stdafx.h"

#include "CodeGen.h"


using namespace llvm;




namespace CodeGenInternal
{

	typedef size_t(__stdcall *ThunkCallbackT)(const wchar_t* thunkname);
	typedef size_t(__stdcall *StringCallbackT)(size_t stringhandle);

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
		TrivialMemoryManager(ThunkCallbackT funcptr, StringCallbackT strptr, uint64_t* outAddr, size_t* outSize, uint64_t* outPData, size_t* outPDataSize, uint64_t* outXData, size_t* outXDataSize)
			: ThunkCallback(funcptr),
			StringCallback(strptr),
			OutAddr(outAddr),
			OutSize(outSize),
			OutPDataOffset(outPData),
			OutPDataSize(outPDataSize),
			OutXDataOffset(outXData),
			OutXDataSize(outXDataSize)
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

	public:
		unsigned GCDataAddress;

	private:		// Internal state
		ThunkCallbackT ThunkCallback;
		StringCallbackT StringCallback;

		uint64_t* OutAddr;
		size_t* OutSize;

		uint64_t* OutPDataOffset;
		size_t* OutPDataSize;
		uint64_t* OutXDataOffset;
		size_t* OutXDataSize;

		SmallVector<sys::MemoryBlock, 16> FunctionMemory;
		SmallVector<sys::MemoryBlock, 16> DataMemory;
	};

	uint8_t* TrivialMemoryManager::allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName)
	{
		std::error_code ec;
		sys::MemoryBlock MB = sys::Memory::allocateMappedMemory(Size, 0, sys::Memory::MF_READ | sys::Memory::MF_WRITE, ec);

		*OutAddr = (uint64_t)MB.base();
		*OutSize = Size;

		FunctionMemory.push_back(MB);
		return (uint8_t*)MB.base();
	}

	uint8_t* TrivialMemoryManager::allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName, bool IsReadOnly)
	{
		std::error_code ec;
		sys::MemoryBlock MB = sys::Memory::allocateMappedMemory(Size, 0, sys::Memory::MF_READ | sys::Memory::MF_WRITE, ec);

		if (SectionName == ".pdata")
		{
			*OutPDataOffset = (uint64_t)MB.base();
			*OutPDataSize = Size;
		}
		else if (SectionName == ".xdata")
		{
			*OutXDataOffset = (uint64_t)MB.base();
			*OutXDataSize = Size;
		}

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
		return 0x401000 + 88;
		//if (foo.substr(0, 21) == "@epoch_static_string:")
		//{
		//	size_t handle = 0;

		//	std::stringstream convert;
		//	convert << foo.substr(21);
		//	convert >> handle;

		//	return StringCallback(handle);
		//}
		//else if (foo == "gcdataoffset")			// TODO - harden this
		//{
		//	return GCDataAddress;
		//}
		//else
		//{
		//	std::wstring wide(foo.begin(), foo.end());
		//	size_t offset = ThunkCallback(wide.c_str());

		//	// TODO - this is a dumb hack
		//	if (offset == 0)
		//		offset = 0x610ba1;

		//	//assert(offset != 0);

		//	return offset;
		//}
	}


	//
	// Batch relocate all .pdata RUNTIME_FUNCTION structures to the correct offsets
	//
	void ProcessPDataRelocations(const object::SectionRef& section, char* buffer, size_t bufferSize, unsigned xdataoffset, unsigned textoffset)
	{
		size_t numrecords = bufferSize / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
		IMAGE_RUNTIME_FUNCTION_ENTRY* data = reinterpret_cast<IMAGE_RUNTIME_FUNCTION_ENTRY*>(buffer);

		for (const auto& reloc : section.relocations())
		{
			uint64_t recordIndex = reloc.getOffset() / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
			uint64_t fieldoffset = reloc.getOffset() % sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);

			switch (fieldoffset)
			{
			case offsetof(IMAGE_RUNTIME_FUNCTION_ENTRY, BeginAddress):
				data[recordIndex].BeginAddress += textoffset;
				break;

			case offsetof(IMAGE_RUNTIME_FUNCTION_ENTRY, EndAddress):
				data[recordIndex].EndAddress += textoffset;
				break;

			case offsetof(IMAGE_RUNTIME_FUNCTION_ENTRY, UnwindInfoAddress):
				data[recordIndex].UnwindInfoAddress += xdataoffset;
				break;
			}
		}
	}


#include <pshpack1.h>
	struct Relocation
	{
		uint32_t address;
		uint32_t symbolindex;
		uint16_t type;
	};
#include <poppack.h>

	template<typename T, size_t TSize = sizeof(T)>
	void AppendToBuffer(std::vector<char>* buffer, const T& data)
	{
		const char* pdata = reinterpret_cast<const char*>(&data);
		for (size_t i = 0; i < TSize; ++i)
		{
			buffer->push_back(*pdata);
			++pdata;
		}
	}

	void ProcessArbitraryRelocations(const object::SectionRef& section, std::vector<char>* buffer)
	{
		for (const auto& reloc : section.relocations())
		{
			unsigned idx = 0;
			for (const auto& sym : section.getObject()->symbols())
			{
				if (sym.getName().get() == reloc.getSymbol()->getName().get())
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

}

using namespace CodeGenInternal;


CodeGenContext::CodeGenContext()
	: LLVMModule(llvm::make_unique<Module>("EpochModule", GlobalContext)),
	  Builder(GlobalContext),
	  DebugBuilder(*LLVMModule)
{
	LLVMModule->setTargetTriple("x86_64-pc-windows-msvc");
	LLVMModule->addModuleFlag(Module::ModFlagBehavior::Warning, "CodeView", 1);

	// TODO - stash CUs for each file of the input program; will require debug info internally in EpochCompiler
	// TODO - change params to handle optimized code when optimizations are back in
	DebugFile = DebugBuilder.createFile("LinkedProgram.epoch", "C:\\Code\\epoch-language");
	DebugCompileUnit = DebugBuilder.createCompileUnit(dwarf::SourceLanguage::DW_LANG_C_plus_plus_11, DebugFile, "Epoch Compiler", false, "", 0);
}



CodeGenContext::~CodeGenContext()
{
}



void CodeGenContext::DebugDump()
{
	LLVMModule->dump();
}



FunctionType* CodeGenContext::TypeCreateFunction()
{
	auto* ret = FunctionType::get(Type::getInt32Ty(GlobalContext), FunctionParamTypeStack, false);
	FunctionParamTypeStack.clear();

	return ret;
}

void CodeGenContext::TypeQueueFunctionParameter(Type* ty)
{
	FunctionParamTypeStack.push_back(ty);
}

llvm::DIType* CodeGenContext::TypeGetDebugType(Type* t)
{
	// TODO - build better type data
	if (t->isPointerTy())
		return DebugBuilder.createPointerType(TypeGetDebugType(t->getPointerElementType()), 64);

	if (t->isStructTy())
		return DebugBuilder.createBasicType("Placeholder", 32, llvm::dwarf::DW_ATE_signed);

	std::string str;
	llvm::raw_string_ostream stream(str);
	t->print(stream, true);

	auto encoding = llvm::dwarf::DW_ATE_signed;
	if (t->getPrimitiveSizeInBits() == 1)
		encoding = llvm::dwarf::DW_ATE_boolean;
	else if (t->getPrimitiveSizeInBits() == 8)
		encoding = llvm::dwarf::DW_ATE_signed_char;

	return DebugBuilder.createBasicType(stream.str(), t->getPrimitiveSizeInBits(), encoding);
}

Type* CodeGenContext::TypeGetString()
{
	return Type::getInt8PtrTy(GlobalContext);
}


Function* CodeGenContext::FunctionCreate(FunctionType* fty, const char* name)
{
	auto* ret = Function::Create(fty, GlobalValue::LinkageTypes::ExternalLinkage, name, LLVMModule.get());

	DIScope* fcontext = DebugCompileUnit;
	unsigned line = 0;
	unsigned scopeline = 0;

	std::vector<Metadata*> argtypes;
	argtypes.push_back(TypeGetDebugType(ret->getReturnType()));

	for (auto& arg : ret->args())
		argtypes.push_back(TypeGetDebugType(arg.getType()));

	DISubroutineType* debugtype = DebugBuilder.createSubroutineType(DebugBuilder.getOrCreateTypeArray(argtypes));

	DISubprogram* subprogram = DebugBuilder.createFunction(fcontext, ret->getName(), StringRef(), DebugFile, line, debugtype, false, true, scopeline, DINode::FlagPrototyped, false);


	unsigned i = 1;
	for (auto& arg : ret->args())
	{
		auto * var = DebugBuilder.createParameterVariable(subprogram, arg.getName(), i, DebugFile, line, (DIType*)(argtypes[i]), false, DINode::DIFlags::FlagZero);
		auto expr = DebugBuilder.createExpression();

		DebugBuilder.insertDeclare(&arg, var, expr, DebugLoc::get(1, 0, subprogram), Builder.GetInsertBlock());

		++i;
	}


	ret->setSubprogram(subprogram);

	return ret;
}

GlobalVariable* CodeGenContext::FunctionCreateThunk(FunctionType* fty, const char* name)
{
	return new GlobalVariable(*LLVMModule, fty->getPointerTo(), true, GlobalValue::ExternalWeakLinkage, NULL, name, NULL, GlobalVariable::NotThreadLocal, 0, true);
}


BasicBlock* CodeGenContext::BasicBlockCreate(Function* func)
{
	return BasicBlock::Create(GlobalContext, "", func);
}

void CodeGenContext::BasicBlockSetInsertPoint(BasicBlock* block)
{
	Builder.SetInsertPoint(block);
}


Value* CodeGenContext::CodeCreateCall(Function* target)
{
	Value* callnode = Builder.CreateCall(target);

	unsigned line = 1;
	unsigned column = 1;
	DebugLoc loc = DILocation::get(GlobalContext, line, column, Builder.GetInsertBlock()->getParent()->getSubprogram());
	if (!Builder.GetInsertBlock()->getInstList().empty())
		Builder.GetInsertBlock()->getInstList().back().setDebugLoc(loc);

	return callnode;
}

Value* CodeGenContext::CodeCreateCallThunk(GlobalVariable* target)
{
	Value* derefTarget = Builder.CreateLoad(target);
	FunctionType* fty = cast<FunctionType>(derefTarget->getType()->getPointerElementType());

	std::vector<Value*> relevantargs;
	for (size_t i = 0; i < fty->getNumParams(); ++i)
	{
		relevantargs.push_back(Builder.CreateIntToPtr(ValueStack.back(), TypeGetString()));
		ValueStack.pop_back();
	}
	std::reverse(relevantargs.begin(), relevantargs.end());

	return Builder.CreateCall(derefTarget, relevantargs);
}

void CodeGenContext::CodeCreateRetVoid()
{
	Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(GlobalContext), 0));
}

void CodeGenContext::CodePushValue()
{
	ValueStack.push_back(ConstantInt::get(Type::getInt64Ty(GlobalContext), 0x405000));
}



void CodeGenContext::CreateBinaryModule()
{
	LLVMLinkInMCJIT();
	LLVMInitializeNativeAsmPrinter();
	LLVMInitializeNativeAsmParser();

	DebugBuilder.finalize();

	std::string errstr;

	TargetOptions opts;
	opts.UnsafeFPMath = true;
	opts.AllowFPOpFusion = FPOpFusion::Fast;
	opts.EnableFastISel = false;
	opts.GuaranteedTailCallOpt = true;

	ThunkCallbackT ThunkCallback = nullptr;
	StringCallbackT StringCallback = nullptr;

	std::unique_ptr<TrivialMemoryManager> blobmgr = std::make_unique<TrivialMemoryManager>(ThunkCallback, StringCallback, &EmissionAddress, &EmissionSize, &EmittedPData, &EmittedPDataSize, &EmittedXData, &EmittedXDataSize);
	
	// HACK! We move the smart pointer's contents into the EngineBuilder
	// below, but we still want to access the module for other purposes.
	Module* llvmmodule = LLVMModule.get();
	CachedMemoryManager = blobmgr.get();


	EngineBuilder eb(std::move(LLVMModule));
	eb.setErrorStr(&errstr);
	eb.setTargetOptions(opts);
	eb.setMCJITMemoryManager(std::move(blobmgr));

	SmallVector<std::string, 2> emptyvec;
	TargetMachine* machine = eb.selectTarget(Triple("x86_64-pc-windows-msvc"), "", "", emptyvec);

	CachedExecutionEngine = eb.create(machine);
	if (!CachedExecutionEngine)
	{
		return;
	}

	llvmmodule->setDataLayout(CachedExecutionEngine->getDataLayout());

	// TODO - reexamine optimizations

	legacy::PassManager mpm;
	mpm.add(createPromoteMemoryToRegisterPass());

	mpm.run(*llvmmodule);

	llvmmodule->dump();

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

			for (const auto & section : img.sections())
			{
				if (section.isText())
				{
					*OutSize += static_cast<size_t>(section.getSize());
				}
			}
		}
	} listener(&EmissionAddress, &EmissionSize, &EmittedImage);

	CachedExecutionEngine->RegisterJITEventListener(&listener);

	CachedExecutionEngine->DisableLazyCompilation(true);
	CachedExecutionEngine->generateCodeForModule(llvmmodule);


	for (const auto& section : EmittedImage->sections())
	{
		if (!section.isText() && !section.isBSS() && !section.isVirtual())
		{
			StringRef sectionname;
			section.getName(sectionname);

			if (sectionname == ".pdata")
			{
				StringRef sectiondata;
				section.getContents(sectiondata);
				std::copy(sectiondata.begin(), sectiondata.end(), std::back_inserter(PData));
			}
			else if (sectionname == ".debug$S")
			{
				StringRef sectiondata;
				section.getContents(sectiondata);
				std::copy(sectiondata.begin(), sectiondata.end(), std::back_inserter(DebugData));
			}
		}
	}
}

void CodeGenContext::RelocateBuffers(unsigned codeOffset, unsigned xDataOffset)
{
	for (const auto& section : EmittedImage->sections())
	{
		if (!section.isText() && !section.isBSS() && !section.isVirtual())
		{
			StringRef sectionname;
			section.getName(sectionname);

			if (sectionname == ".pdata")
			{
				ProcessPDataRelocations(section, PData.data(), PData.size(), xDataOffset, codeOffset);
			}
			else if (sectionname == ".debug$S")
			{
				ProcessArbitraryRelocations(section, &DebugRelocs);
			}
		}
	}

	DebugSymbolCount = 0;
	std::vector<char> stringbuffer;

	uint32_t offset = 4;
	for (const auto& sym : EmittedImage->symbols())
	{
		IMAGE_SYMBOL symbol;

		memset(symbol.N.ShortName, 0, 8);

		auto nameerr = sym.getName();
		if (!nameerr)
		{
			std::cout << "SKIP nameless symbol" << std::endl;
			continue;
		}

		auto nameref = nameerr.get();
		auto symname = nameref.str();

		symbol.N.LongName[1] = offset;


		std::copy(std::begin(symname), std::end(symname), std::back_inserter(stringbuffer));
		stringbuffer.push_back(0);
		offset += symname.length() + 1;

		symbol.Value = (DWORD)sym.getValue();
		symbol.SectionNumber = IMAGE_SYM_ABSOLUTE;
		symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;

		switch (sym.getType().get())
		{
		case object::SymbolRef::ST_Function:
			std::cout << "Function: " << symname << std::endl;
			symbol.SectionNumber = 9;
			symbol.Type = (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
			break;

		default:
			std::cout << "Symbol: " << symname << std::endl;
			symbol.Value = 0;
			symbol.Type = (IMAGE_SYM_DTYPE_POINTER << N_BTSHFT);
			break;
		}

		symbol.NumberOfAuxSymbols = 0;

		AppendToBuffer<IMAGE_SYMBOL, IMAGE_SIZEOF_SYMBOL>(&DebugSymbols, symbol);
		++DebugSymbolCount;
	}

	AppendToBuffer(&DebugSymbols, uint32_t(stringbuffer.size() + 8));

	std::copy(std::begin(stringbuffer), std::end(stringbuffer), std::back_inserter(DebugSymbols));
}

void CodeGenContext::FinalizeBinaryModule(unsigned moduleBaseAddress, unsigned codeOffset)
{
	CachedMemoryManager->GCDataAddress = 0;
	CachedExecutionEngine->mapSectionAddress((void*)EmissionAddress, moduleBaseAddress + codeOffset);
	CachedExecutionEngine->finalizeObject();

	CodeBuffer.resize(EmissionSize);
	memcpy(CodeBuffer.data(), (void*)(EmissionAddress), EmissionSize);
}

void* CodeGenContext::GetCodeBuffer(unsigned* outSize)
{
	if (outSize)
		*outSize = (unsigned)(CodeBuffer.size());

	return (void*)(CodeBuffer.data());
}


void* CodeGenContext::GetDebugBuffer(unsigned* outSize)
{
	if (outSize)
		*outSize = (unsigned)(DebugData.size());

	return (void*)(DebugData.data());
}

void* CodeGenContext::GetDebugRelocBuffer(unsigned* outSize)
{
	if (outSize)
		*outSize = (unsigned)(DebugRelocs.size());

	return (void*)(DebugRelocs.data());
}

void* CodeGenContext::GetDebugSymbolsBuffer(unsigned* outSize, unsigned* outCount)
{
	if (outSize)
		*outSize = (unsigned)(DebugSymbols.size());

	if (outCount)
		*outCount = DebugSymbolCount;

	return (void*)(DebugSymbols.data());
}

void* CodeGenContext::GetPDataBuffer(unsigned* outSize)
{
	if (outSize)
		*outSize = (unsigned)(PData.size());

	return PData.data();
}


void* CodeGenContext::GetXDataBuffer(unsigned* outSize)
{
	if (outSize)
		*outSize = (unsigned)(EmittedXDataSize);

	return (void*)(EmittedXData);
}

