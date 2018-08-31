#include "stdafx.h"

#include "CodeGen.h"


using namespace llvm;



CodeGenContext::CodeGenContext()
	: LLVMModule(std::make_unique<Module>("EpochModule", GlobalContext))
{
}



CodeGenContext::~CodeGenContext()
{
}



void CodeGenContext::DebugDump()
{
	LLVMModule->dump();
}

