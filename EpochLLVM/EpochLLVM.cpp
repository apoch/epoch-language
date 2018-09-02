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


	void EpochLLVMModuleDump(CodeGenContext* context)
	{
		context->DebugDump();
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

	
	void EpochLLVMCodeCreateRetVoid(CodeGenContext* context)
	{
		context->CodeCreateRetVoid();
	}

}


