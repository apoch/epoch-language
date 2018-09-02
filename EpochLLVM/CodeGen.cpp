#include "stdafx.h"

#include "CodeGen.h"


using namespace llvm;



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
	return FunctionType::get(Type::getVoidTy(GlobalContext), false);
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
	Builder.CreateRetVoid();
}

