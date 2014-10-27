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


