//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for invoking generated CUDA code
//

#include "pch.h"

#include "CUDA Wrapper/InvokeCode.h"
#include "CUDA Wrapper/VariableBuffer.h"
#include "CUDA Wrapper/FunctionCall.h"
#include "CUDA Wrapper/Module.h"
#include "CUDA Wrapper/Naming.h"

#include "Code Generation/CompiledCodeManager.h"

#include "Utility/Strings.h"

#include "Utility/Threading/Synchronization.h"


namespace { Threads::CriticalSection InvocationCriticalSection; }


//
// Construct and initialize the execution wrapper
//
CUDACodeInvoker::CUDACodeInvoker(Extensions::CodeBlockHandle codehandle, HandleType activatedscopehandle)
	: ActivatedScopeHandle(activatedscopehandle),
	  CodeHandle(codehandle),
	  RegisteredVariables(Compiler::GetRegisteredVariables(codehandle)),
	  FunctionName(GenerateFunctionName(Compiler::GetOriginalCodeHandle(codehandle)))
{
}


//
// Execute the bound CUDA code block
//
void CUDACodeInvoker::Execute()
{
	VariableBuffer varbuffer(RegisteredVariables);
	varbuffer.CopyToDevice(ActivatedScopeHandle);

	{
		Threads::CriticalSection::Auto mutex(InvocationCriticalSection);

		FunctionCall call = Module::LoadCUDAModule(narrow(Compiler::GetGeneratedPTXFileName(Compiler::GetAssociatedSession(CodeHandle)))).CreateFunctionCall(FunctionName);
		varbuffer.PrepareFunctionCall(call);
		call.Execute();
	}

	varbuffer.CopyFromDevice(ActivatedScopeHandle);
}


