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
		TrivialMemoryManager(ThunkCallbackT funcptr, StringCallbackT strptr, uint64_t* outAddr, size_t* outSize, uint64_t* outPData, uint64_t* outXData)
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

	public:
		unsigned GCDataAddress;

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
			*OutPDataOffset = (uint64_t)MB.base();
		else if (SectionName == ".xdata")
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
		return 0xfabf00;
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


}

using namespace CodeGenInternal;


CodeGenContext::CodeGenContext()
	: LLVMModule(llvm::make_unique<Module>("EpochModule", GlobalContext)),
	  Builder(GlobalContext)
{
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
	return FunctionType::get(Type::getInt32Ty(GlobalContext), false);
}


Function* CodeGenContext::FunctionCreate(FunctionType* fty)
{
	return Function::Create(fty, GlobalValue::LinkageTypes::ExternalLinkage, "entrypoint", LLVMModule.get());
}


BasicBlock* CodeGenContext::BasicBlockCreate(Function* func)
{
	return BasicBlock::Create(GlobalContext, "", func);
}

void CodeGenContext::BasicBlockSetInsertPoint(BasicBlock* block)
{
	Builder.SetInsertPoint(block);
}



void CodeGenContext::CodeCreateRetVoid()
{
	Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(GlobalContext), 0));
}



void CodeGenContext::CreateBinaryModule()
{
	LLVMLinkInMCJIT();
	LLVMInitializeNativeAsmPrinter();
	LLVMInitializeNativeAsmParser();


	std::string errstr;

	TargetOptions opts;
	opts.UnsafeFPMath = true;
	opts.AllowFPOpFusion = FPOpFusion::Fast;
	opts.EnableFastISel = false;
	opts.GuaranteedTailCallOpt = true;

	ThunkCallbackT ThunkCallback = nullptr;
	StringCallbackT StringCallback = nullptr;

	uint64_t pdata = 0;
	uint64_t xdata = 0;
	std::unique_ptr<TrivialMemoryManager> blobmgr = std::make_unique<TrivialMemoryManager>(ThunkCallback, StringCallback, &EmissionAddress, &EmissionSize, &pdata, &xdata);
	
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
			std::vector<char>* targetbuffer = nullptr;

			StringRef sectionname;
			section.getName(sectionname);

			if (sectionname == ".pdata")
				targetbuffer = &PData;
			else if (sectionname == ".xdata")
				targetbuffer = &XData;
			else if (sectionname == ".debug$S")
				targetbuffer = &DebugData;

			if (targetbuffer)
			{
				StringRef sectiondata;
				section.getContents(sectiondata);
				std::copy(sectiondata.begin(), sectiondata.end(), std::back_inserter(*targetbuffer));
			}
		}
	}
}

void CodeGenContext::FinalizeBinaryModule(unsigned codeBaseAddress)
{
	CachedMemoryManager->GCDataAddress = 0;
	CachedExecutionEngine->mapSectionAddress((void*)EmissionAddress, codeBaseAddress);
	CachedExecutionEngine->finalizeObject();

	for (const auto& section : EmittedImage->sections())
	{
		if (section.isText())
		{
			StringRef sectiondata;
			section.getContents(sectiondata);
			std::copy(sectiondata.begin(), sectiondata.end(), std::back_inserter(CodeBuffer));
		}
	}
}

void* CodeGenContext::GetCodeBuffer(unsigned* outSize)
{
	if (outSize)
		*outSize = (unsigned)(CodeBuffer.size());

	return (void*)(CodeBuffer.data());
}

