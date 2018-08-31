// EpochLLVM.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


#include "CodeGen.h"

extern "C"
{

	void* EpochLLVMContextCreate()
	{
		return new CodeGenContext;
	}

	void EpochLLVMContextDestroy(CodeGenContext* context)
	{
		delete context;
	}


	void EpochLLVMModuleDump(CodeGenContext* context)
	{
		context->DebugDump();
	}

}


