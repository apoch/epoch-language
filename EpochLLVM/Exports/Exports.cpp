//
// The Epoch Language Project
// Epoch Development Tools - LLVM wrapper library
//
// EXPORTS.CPP
// C-API exports from the library (for use by Epoch programs)
//


#include "Pch.h"

#include "../LLVM Wrappers/CodeGenContext.h"


extern "C" void EpochLLVMInitialize()
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
}


extern "C" void* EpochLLVMContextCreate()
{
	return new CodeGen::Context();
}

extern "C" void EpochLLVMContextDestroy(void* context)
{
	delete reinterpret_cast<CodeGen::Context*>(context);
}


extern "C" void* EpochLLVMFunctionTypeCreate(void* context, void* rettype)
{
	return reinterpret_cast<CodeGen::Context*>(context)->FunctionTypeCreate(reinterpret_cast<llvm::Type*>(rettype));
}

extern "C" void EpochLLVMFunctionTypePush(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->FunctionTypePush();
}


extern "C" void* EpochLLVMTypeGetBoolean(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetBoolean();
}

extern "C" void* EpochLLVMTypeGetInteger(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetInteger();
}

extern "C" void* EpochLLVMTypeGetInteger16(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetInteger16();
}

extern "C" void* EpochLLVMTypeGetInteger64(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetInteger64();
}

extern "C" void* EpochLLVMTypeGetPointerTo(void* context, void* ty)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetPointerTo(reinterpret_cast<llvm::Type*>(ty));
}

extern "C" void* EpochLLVMTypeGetReal(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetReal();
}

extern "C" void* EpochLLVMTypeGetString(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetString();
}

extern "C" void* EpochLLVMTypeGetVoid(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetVoid();
}

extern "C" void* EpochLLVMTypeGetBuffer(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->TypeGetBuffer();
}



extern "C" void* EpochLLVMFunctionCreate(void* context, const wchar_t* name, void* ftype)
{
	std::wstring widename(name);
	std::string narrowname(widename.begin(), widename.end());
	return reinterpret_cast<CodeGen::Context*>(context)->FunctionCreate(narrowname.c_str(), reinterpret_cast<llvm::FunctionType*>(ftype));
}

extern "C" void* EpochLLVMFunctionCreateThunk(void* context, const wchar_t* name, void* ftype)
{
	std::wstring widename(name);
	std::string narrowname(widename.begin(), widename.end());
	return reinterpret_cast<CodeGen::Context*>(context)->FunctionCreateThunk(narrowname.c_str(), reinterpret_cast<llvm::FunctionType*>(ftype));
}

extern "C" void EpochLLVMFunctionFinalize(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->FunctionFinalize();
}

extern "C" void EpochLLVMFunctionQueueParamType(void* context, void* type)
{
	reinterpret_cast<CodeGen::Context*>(context)->FunctionQueueParamType(reinterpret_cast<llvm::Type*>(type));
}


extern "C" void EpochLLVMFunctionSetEntry(void* context, void* func)
{
	reinterpret_cast<CodeGen::Context*>(context)->SetEntryFunction(reinterpret_cast<llvm::Function*>(func));
}



extern "C" size_t EpochLLVMEmitBinaryObject(void* context, char* buffer, size_t maxoutput)
{
	return reinterpret_cast<CodeGen::Context*>(context)->EmitBinaryObject(buffer, maxoutput);
}

extern "C" void EpochLLVMSetThunkCallback(void* context, void* funcptr)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SetThunkCallback(funcptr);
}

extern "C" void EpochLLVMSetStringCallback(void* context, void* funcptr)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SetStringCallback(funcptr);
}


extern "C" void* EpochLLVMCodeCreateAlloca(void* context, void* vartype, const wchar_t* varname)
{
	std::wstring widename(varname);
	std::string narrowname(widename.begin(), widename.end());

	return reinterpret_cast<CodeGen::Context*>(context)->CodeCreateAlloca(reinterpret_cast<llvm::Type*>(vartype), narrowname.c_str());
}

extern "C" void* EpochLLVMCodeCreateBasicBlock(void* context, void* parentfunc, bool setinsertpoint)
{
	return reinterpret_cast<CodeGen::Context*>(context)->CodeCreateBasicBlock(reinterpret_cast<llvm::Function*>(parentfunc), setinsertpoint);
}

extern "C" void EpochLLVMCodeCreateBranch(void* context, void* target, bool setinsertpoint)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateBranch(reinterpret_cast<llvm::BasicBlock*>(target), setinsertpoint);
}

extern "C" void* EpochLLVMCodeCreateCall(void* context, void* target)
{
	return reinterpret_cast<CodeGen::Context*>(context)->CodeCreateCall(reinterpret_cast<llvm::Function*>(target));
}

extern "C" void EpochLLVMCodeCreateCallIndirect(void* context, void* target)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateCallIndirect(reinterpret_cast<llvm::AllocaInst*>(target));
}

extern "C" void* EpochLLVMCodeCreateCallThunk(void* context, void* target)
{
	return reinterpret_cast<CodeGen::Context*>(context)->CodeCreateCallThunk(reinterpret_cast<llvm::GlobalVariable*>(target));
}

extern "C" void EpochLLVMCodeCreateCondBranch(void* context, void* cond, void* truetarget, void* falsetarget)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateCondBranch(reinterpret_cast<llvm::Value*>(cond), reinterpret_cast<llvm::BasicBlock*>(truetarget), reinterpret_cast<llvm::BasicBlock*>(falsetarget));
}

extern "C" void* EpochLLVMCodeCreateGEP(void* context, unsigned index)
{
	return reinterpret_cast<CodeGen::Context*>(context)->CodeCreateGEP(index);
}

extern "C" void EpochLLVMCodeCreateRead(void* context, void* allocatarget)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateRead(reinterpret_cast<llvm::AllocaInst*>(allocatarget));
}

extern "C" void EpochLLVMCodeCreateReadParam(void* context, unsigned index)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateReadParam(index);
}

extern "C" void EpochLLVMCodeCreateReadStructure(void* context, void* gep)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateReadStructure(reinterpret_cast<llvm::Value*>(gep));
}

extern "C" void EpochLLVMCodeCreateRet(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateRet();
}

extern "C" void EpochLLVMCodeCreateRetVoid(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateRetVoid();
}

extern "C" void EpochLLVMCodeCreateWrite(void* context, void* allocatarget)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateWrite(reinterpret_cast<llvm::AllocaInst*>(allocatarget));
}

extern "C" void EpochLLVMCodeCreateWriteParam(void* context, unsigned index)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateWriteParam(index);
}

extern "C" void EpochLLVMCodeCreateWriteStructure(void* context, void* gep)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateWriteStructure(reinterpret_cast<llvm::Value*>(gep));
}

extern "C" void EpochLLVMCodeCreateWriteStructurePop(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateWriteStructurePop();
}


extern "C" void EpochLLVMCodeCreateDereference(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateDereference();
}


extern "C" void EpochLLVMCodeOperatorBooleanNot(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateOperatorBooleanNot();
}

extern "C" void EpochLLVMCodeOperatorIntegerEquals(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateOperatorIntegerEquals();
}

extern "C" void EpochLLVMCodeOperatorIntegerNotEquals(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateOperatorIntegerNotEquals();
}

extern "C" void EpochLLVMCodeOperatorIntegerPlus(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateOperatorIntegerPlus();
}

extern "C" void EpochLLVMCodeOperatorIntegerMinus(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateOperatorIntegerMinus();
}



extern "C" void EpochLLVMCodePushBoolean(void* context, bool value)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushBoolean(value);
}

extern "C" void EpochLLVMCodePushInteger(void* context, int value)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushInteger(value);
}

extern "C" void EpochLLVMCodePushInteger16(void* context, short value)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushInteger16(value);
}

extern "C" void EpochLLVMCodePushInteger64(void* context, int value)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushInteger64(value);
}


extern "C" void EpochLLVMCodePushRawAlloca(void* context, void* alloc)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushRawAlloca(reinterpret_cast<llvm::AllocaInst*>(alloc));
}

extern "C" void EpochLLVMCodePushRawCall(void* context, void* callinst)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushRawCall(reinterpret_cast<llvm::CallInst*>(callinst));
}

extern "C" void EpochLLVMCodePushRawGEP(void* context, void* gep)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushRawGEP(reinterpret_cast<llvm::Value*>(gep));
}

extern "C" void EpochLLVMCodePushString(void* context, unsigned handle)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushString(handle);
}

extern "C" void EpochLLVMCodePushFunction(void* context, void* func)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushFunction(reinterpret_cast<llvm::Function*>(func));
}


extern "C" void EpochLLVMCodeStatementFinalize(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeStatementFinalize();
}



extern "C" void* EpochLLVMGetCurrentBasicBlock(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->GetCurrentBasicBlock();
}

extern "C" void EpochLLVMSetCurrentBasicBlock(void* context, void* block)
{
	reinterpret_cast<CodeGen::Context*>(context)->SetCurrentBasicBlock(reinterpret_cast<llvm::BasicBlock*>(block));
}



extern "C" void* EpochLLVMStructureTypeCreate(void* context, const wchar_t* name)
{
	std::wstring widename(name);
	std::string narrowname(widename.begin(), widename.end());
	return reinterpret_cast<CodeGen::Context*>(context)->StructureTypeCreate(narrowname.c_str());
}

extern "C" void EpochLLVMStructureQueueMemberType(void* context, void* membertype)
{
	reinterpret_cast<CodeGen::Context*>(context)->StructureTypeQueueMember(reinterpret_cast<llvm::Type*>(membertype));
}



extern "C" void EpochLLVMSectionCopyPData(void* context, void* buffer)
{
	reinterpret_cast<CodeGen::Context*>(context)->SectionCopyPData(buffer);
}

extern "C" void EpochLLVMSectionCopyXData(void* context, void* buffer)
{
	reinterpret_cast<CodeGen::Context*>(context)->SectionCopyXData(buffer);
}

extern "C" void EpochLLVMSectionCopyGC(void* context, void* buffer)
{
	reinterpret_cast<CodeGen::Context*>(context)->SectionCopyGC(buffer);
}

extern "C" void EpochLLVMSectionCopyDebug(void* context, void* buffer)
{
	reinterpret_cast<CodeGen::Context*>(context)->SectionCopyDebug(buffer);
}

extern "C" void EpochLLVMSectionCopyDebugReloc(void* context, void* buffer)
{
	reinterpret_cast<CodeGen::Context*>(context)->SectionCopyDebugReloc(buffer);
}

extern "C" void EpochLLVMSectionCopyDebugSymbols(void* context, void* buffer)
{
	reinterpret_cast<CodeGen::Context*>(context)->SectionCopyDebugSymbols(buffer);
}



extern "C" unsigned EpochLLVMSectionGetPDataSize(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SectionGetPDataSize();
}

extern "C" unsigned EpochLLVMSectionGetXDataSize(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SectionGetXDataSize();
}

extern "C" unsigned EpochLLVMSectionGetGCSize(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SectionGetGCSize();
}

extern "C" unsigned EpochLLVMSectionGetDebugSize(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SectionGetDebugSize();
}

extern "C" unsigned EpochLLVMSectionGetDebugRelocSize(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SectionGetDebugRelocSize();
}

extern "C" unsigned EpochLLVMSectionGetDebugSymbolSize(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->SectionGetDebugSymbolSize();
}



extern "C" void* EpochLLVMCodePopValue(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->CodePopValue();
}
