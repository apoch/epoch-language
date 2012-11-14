//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for debugging purposes
//

#include "pch.h"

#include "Library Functionality/Debugging/Debugging.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "User Interface/Output.h"
#include "User Interface/Input.h"

#include "Virtual Machine/VirtualMachine.h"



namespace
{

	unsigned* TestHarness = NULL;


	//
	// Write a string to the debug output
	//
	void WriteString(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle handle = context.State.Stack.PopValue<StringHandle>();

		UI::OutputStream stream;
		stream << UI::lightblue << L"DEBUG: " << UI::resetcolor << context.OwnerVM.GetPooledString(handle) << std::endl;
	}

	//
	// Read a string from the debug console
	//
	void ReadString(StringHandle, VM::ExecutionContext& context)
	{
		UI::Input input;
		StringHandle handle = context.OwnerVM.PoolString(input.BlockingRead());
		context.State.Stack.PushValue(handle);
		context.TickStringGarbageCollector();
	}

	//
	// Simple assertion mechanism
	//
	void Assert(StringHandle, VM::ExecutionContext& context)
	{
		bool value = context.State.Stack.PopValue<bool>();

		if(!value)
		{
			context.State.Result.ResultType = VM::ExecutionResult::EXEC_RESULT_HALT;
			UI::OutputStream output;
			output << UI::lightred << L"Assertion failure" << UI::white << std::endl;
		}
	}

	//
	// Test harness
	//
	void PassTest(StringHandle, VM::ExecutionContext&)
	{
		UI::OutputStream stream;
		stream << UI::lightblue << L"TEST: " << UI::resetcolor << L"Pass" << std::endl;

		if(TestHarness)
			++(*TestHarness);
	}
}


//
// Bind the library to an execution dispatch table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"print"), WriteString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"read"), ReadString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"assert"), Assert));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"passtest"), PassTest));
}

//
// Bind the library to a function metadata table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"print"), signature));
	}
	{
		FunctionSignature signature;
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"read"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"value", Metadata::EpochType_Boolean, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"assert"), signature));
	}
	{
		FunctionSignature signature;
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"passtest"), signature));
	}
}


void DebugLibrary::LinkToTestHarness(unsigned* harness)
{
	TestHarness = harness;
}

