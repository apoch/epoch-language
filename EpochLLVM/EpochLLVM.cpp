// EpochLLVM.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


#include "CodeGen.h"

extern "C"
{

	void* EpochLLVMContextCreate()
	{
		llvm::InitializeNativeTarget();
		return new CodeGenContext;
	}

	void EpochLLVMContextDestroy(CodeGenContext* context)
	{
		delete context;
		llvm::llvm_shutdown();
	}


	void EpochLLVMModuleCreateBinary(CodeGenContext* context)
	{
		context->CreateBinaryModule();
	}

	void EpochLLVMModuleFinalize(CodeGenContext* context, unsigned moduleBaseAddress, unsigned codeOffset)
	{
		context->FinalizeBinaryModule(moduleBaseAddress, codeOffset);
	}

	void EpochLLVMModuleDump(CodeGenContext* context)
	{
		context->DebugDump();
	}

	void EpochLLVMModuleRelocateBuffers(CodeGenContext* context, unsigned codeOffset, unsigned xDataOffset)
	{
		context->RelocateBuffers(codeOffset, xDataOffset);
	}

	void* EpochLLVMModuleGetCodeBuffer(CodeGenContext* context, unsigned* outSize)
	{
		return context->GetCodeBuffer(outSize);
	}

	void* EpochLLVMModuleGetPDataBuffer(CodeGenContext* context, unsigned* outSize)
	{
		return context->GetPDataBuffer(outSize);
	}

	void* EpochLLVMModuleGetXDataBuffer(CodeGenContext* context, unsigned* outSize)
	{
		return context->GetXDataBuffer(outSize);
	}


	llvm::FunctionType* EpochLLVMTypeCreateFunction(CodeGenContext* context)
	{
		return context->TypeCreateFunction();
	}

	llvm::Function* EpochLLVMFunctionCreate(CodeGenContext* context, llvm::FunctionType* fty)
	{
		return context->FunctionCreate(fty);
	}

	llvm::BasicBlock* EpochLLVMBasicBlockCreate(CodeGenContext* context, llvm::Function* func)
	{
		return context->BasicBlockCreate(func);
	}


	void EpochLLVMBasicBlockSetInsertPoint(CodeGenContext* context, llvm::BasicBlock* block)
	{
		context->BasicBlockSetInsertPoint(block);
	}

	
	llvm::Value* EpochLLVMCodeCreateCall(CodeGenContext* context, llvm::Function* func)
	{
		return context->CodeCreateCall(func);
	}

	void EpochLLVMCodeCreateRetVoid(CodeGenContext* context)
	{
		context->CodeCreateRetVoid();
	}

}


