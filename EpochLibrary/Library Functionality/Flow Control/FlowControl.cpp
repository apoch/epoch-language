//
// The Epoch Language Project
// Epoch Standard Library
//
// Additional flow control keywords
//

#include "pch.h"

#include "Library Functionality/Flow Control/FlowControl.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Virtual Machine/VirtualMachine.h"



namespace
{

	//
	// Return to the calling context
	//
	void Return(StringHandle, VM::ExecutionContext& context)
	{
		context.State.Result.ResultType = VM::ExecutionResult::EXEC_RESULT_RETURN;
	}

}


//
// Bind the library to an execution dispatch table
//
void FlowControl::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"return"), Return));
}

//
// Bind the library to a function metadata table
//
void FlowControl::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.SetReturnType(VM::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"return"), signature));
	}
}

