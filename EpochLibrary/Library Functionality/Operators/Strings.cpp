//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for string-handling operators
//

#include "pch.h"

#include "Library Functionality/Operators/Strings.h"
#include "Library Functionality/Operators/Precedences.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Virtual Machine/VirtualMachine.h"



namespace
{
	void StringConcatenation(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle p2 = context.State.Stack.PopValue<StringHandle>();
		StringHandle p1 = context.State.Stack.PopValue<StringHandle>();

		std::wstring ret = context.OwnerVM.GetPooledString(p1) + context.OwnerVM.GetPooledString(p2);
		StringHandle rethandle = context.OwnerVM.PoolString(ret);

		context.State.Stack.PushValue(rethandle);
	}
}


//
// Bind the library to an execution dispatch table
//
void StringLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L";"), StringConcatenation));
}

//
// Bind the library to a function metadata table
//
void StringLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"s1", VM::EpochType_String);
		signature.AddParameter(L"s2", VM::EpochType_String);
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L";"), signature));
	}
}

//
// Bind the library to the infix operator table
//
void StringLibrary::RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool)
{
	{
		StringHandle handle = stringpool.Pool(L";");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_CONCATENATION, handle));
	}
}

