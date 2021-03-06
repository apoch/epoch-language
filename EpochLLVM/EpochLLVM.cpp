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

	void* EpochLLVMModuleGetDebugBuffer(CodeGenContext* context, unsigned* outSize)
	{
		return context->GetDebugBuffer(outSize);
	}

	void* EpochLLVMModuleGetDebugRelocBuffer(CodeGenContext* context, unsigned* outSize)
	{
		return context->GetDebugRelocBuffer(outSize);
	}

	void* EpochLLVMModuleGetDebugSymbolsBuffer(CodeGenContext* context, unsigned* outSize, unsigned* outCount)
	{
		return context->GetDebugSymbolsBuffer(outSize, outCount);
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

	void EpochLLVMTypeQueueFunctionParameter(CodeGenContext* context, llvm::Type* ty)
	{
		context->TypeQueueFunctionParameter(ty);
	}

	llvm::GlobalVariable* EpochLLVMFunctionCreateThunk(CodeGenContext* context, llvm::FunctionType* fty, const wchar_t* wideName)
	{
		std::wstring wideStr(wideName);
		std::string name(wideStr.begin(), wideStr.end());
		return context->FunctionCreateThunk(fty, name.c_str());
	}

	llvm::Function* EpochLLVMFunctionCreate(CodeGenContext* context, llvm::FunctionType* fty, const wchar_t* wideName)
	{
		std::wstring wideStr(wideName);
		std::string name(wideStr.begin(), wideStr.end());
		return context->FunctionCreate(fty, name.c_str());
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

	llvm::Value* EpochLLVMCodeCreateCallThunk(CodeGenContext* context, llvm::GlobalVariable* func)
	{
		return context->CodeCreateCallThunk(func);
	}

	void EpochLLVMCodeCreateRetVoid(CodeGenContext* context)
	{
		context->CodeCreateRetVoid();
	}

	void EpochLLVMCodePushValue(CodeGenContext* context, llvm::Value* value)
	{
		context->CodePushValue(value);
	}

	llvm::Value* EpochLLVMCodeGetStringValue(CodeGenContext* context, unsigned index)
	{
		return context->GetStringPoolEntry(index);
	}

	llvm::Type* EpochLLVMTypeGetString(CodeGenContext* context)
	{
		return context->TypeGetString();
	}

	void EpochLLVMContextSetStringPoolCallback(CodeGenContext* context, void* functionPointer)
	{
		context->SetStringPoolCallback(functionPointer);
	}

}


