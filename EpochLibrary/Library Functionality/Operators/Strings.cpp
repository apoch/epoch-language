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
	void StringConcatenation(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle p2 = context.State.Stack.PopValue<StringHandle>();
		StringHandle p1 = context.State.Stack.PopValue<StringHandle>();

		std::wstring ret = context.OwnerVM.GetPooledString(p1) + context.OwnerVM.GetPooledString(p2);
		StringHandle rethandle = context.OwnerVM.PoolStringDestructive(ret);

		context.State.Stack.PushValue(rethandle);
		context.TickStringGarbageCollector();
	}

	void StringLength(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle p = context.State.Stack.PopValue<StringHandle>();
		context.State.Stack.PushValue(context.OwnerVM.GetPooledString(p).length());
	}

	void NarrowString(StringHandle functionname, VM::ExecutionContext& context)
	{
		void* bufferrefstorage = context.State.Stack.PopValue<void*>();
		VM::EpochTypeID bufferreftype = context.State.Stack.PopValue<VM::EpochTypeID>();

		if(bufferreftype != VM::EpochType_Buffer)
			throw FatalException("Passed a non-buffer parameter as a reference to narrowstring()");

		BufferHandle destbuffer = *reinterpret_cast<BufferHandle*>(bufferrefstorage);
		StringHandle sourcestringhandle = context.State.Stack.PopValue<StringHandle>();

		const std::wstring& sourcestring = context.OwnerVM.GetPooledString(sourcestringhandle);

		std::string narrowed(narrow(sourcestring));

		if(context.OwnerVM.GetBufferSize(destbuffer) <= narrowed.length())
			throw FatalException("Buffer overflow during string narrow conversion");

		memcpy(context.OwnerVM.GetBuffer(destbuffer), narrowed.c_str(), narrowed.length());
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
		signature.AddParameter(L"b", VM::EpochType_Buffer, true);
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
		precedences.insert(std::make_pair(PRECEDENCE_CONCATENATION, handle));
	}
}

