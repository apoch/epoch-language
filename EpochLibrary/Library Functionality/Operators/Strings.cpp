//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for string-handling operators
//

#include "pch.h"

#include "Library Functionality/Operators/Strings.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"
#include "Utility/Strings.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/Precedences.h"


namespace
{
	//
	// Concatenate two strings and return the result
	//
	// Note that this function allocates a new string and therefore should tick the
	// garbage collector to ensure that the allocation is taken into account when it
	// comes time to decide when to collect garbage again.
	//
	void StringConcatenation(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle p2 = context.State.Stack.PopValue<StringHandle>();
		StringHandle p1 = context.State.Stack.PopValue<StringHandle>();

		std::wstring ret = context.OwnerVM.GetPooledString(p1) + context.OwnerVM.GetPooledString(p2);
		StringHandle rethandle = context.OwnerVM.PoolString(ret);

		context.State.Stack.PushValue(rethandle);
		context.TickStringGarbageCollector();
	}

	//
	// Retrieve the length (in characters, not bytes!) of a string
	//
	void StringLength(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle p = context.State.Stack.PopValue<StringHandle>();
		context.State.Stack.PushValue(context.OwnerVM.GetPooledString(p).length());
	}

	//
	// Narrow a string into a byte buffer; typically useful for marshaling strings to APIs that expect narrow strings
	//
	void NarrowString(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle sourcestringhandle = context.State.Stack.PopValue<StringHandle>();

		const std::wstring& sourcestring = context.OwnerVM.GetPooledString(sourcestringhandle);

		std::string narrowed(narrow(sourcestring));

		BufferHandle destbuffer = context.OwnerVM.AllocateBuffer(narrowed.length() + 1);
		void* storage = context.OwnerVM.GetBuffer(destbuffer);
		memcpy(storage, narrowed.c_str(), narrowed.length());

		context.State.Stack.PushValue(destbuffer);
		context.TickBufferGarbageCollector();
	}
}


//
// Bind the library to an execution dispatch table
//
void StringLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L";"), StringConcatenation));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"length"), StringLength));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"narrowstring"), NarrowString));
}

//
// Bind the library to a function metadata table
//
void StringLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"s1", VM::EpochType_String, false);
		signature.AddParameter(L"s2", VM::EpochType_String, false);
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L";"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"s", VM::EpochType_String, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"length"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"s", VM::EpochType_String, false);
		signature.SetReturnType(VM::EpochType_Buffer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"narrowstring"), signature));
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
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_CONCATENATION));
	}
}

