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

extern "C" void* EpochLLVMCodeCreateCallThunk(void* context, void* target)
{
	return reinterpret_cast<CodeGen::Context*>(context)->CodeCreateCallThunk(reinterpret_cast<llvm::GlobalVariable*>(target));
}

extern "C" void EpochLLVMCodeCreateCondBranch(void* context, void* truetarget, void* falsetarget)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateCondBranch(reinterpret_cast<llvm::BasicBlock*>(truetarget), reinterpret_cast<llvm::BasicBlock*>(falsetarget));
}

extern "C" void EpochLLVMCodeCreateRet(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateRet();
}

extern "C" void EpochLLVMCodeCreateRetVoid(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateRetVoid();
}


extern "C" void EpochLLVMCodeOperatorBooleanNot(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateOperatorBooleanNot();
}

extern "C" void EpochLLVMCodeOperatorIntegerEquals(void* context)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodeCreateOperatorIntegerEquals();
}



extern "C" void EpochLLVMCodePushBoolean(void* context, bool value)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushBoolean(value);
}

extern "C" void EpochLLVMCodePushInteger(void* context, int value)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushInteger(value);
}

extern "C" void EpochLLVMCodePushString(void* context, unsigned handle)
{
	reinterpret_cast<CodeGen::Context*>(context)->CodePushString(handle);
}



extern "C" void* EpochLLVMGetCurrentBasicBlock(void* context)
{
	return reinterpret_cast<CodeGen::Context*>(context)->GetCurrentBasicBlock();
}

extern "C" void EpochLLVMSetCurrentBasicBlock(void* context, void* block)
{
	reinterpret_cast<CodeGen::Context*>(context)->SetCurrentBasicBlock(reinterpret_cast<llvm::BasicBlock*>(block));
}

