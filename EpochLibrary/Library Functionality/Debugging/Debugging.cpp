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


	//
	// Stupid hack
	// TODO - replace this with generic array support
	//
	void PlotPixel(StringHandle, VM::ExecutionContext& context)
	{
		UInteger32 pixelcolor = context.State.Stack.PopValue<UInteger32>();
		UInteger32 offset = context.State.Stack.PopValue<UInteger32>();
		void* phandle = context.State.Stack.PopValue<void*>();
		context.State.Stack.PopValue<Metadata::EpochTypeID>();

		BufferHandle handle = *reinterpret_cast<BufferHandle*>(phandle);

		*(reinterpret_cast<UInteger32*>(context.OwnerVM.GetBuffer(handle)) + offset) = pixelcolor;
	}

	// TODO - move this to a better home
	void Sqrt(StringHandle, VM::ExecutionContext& context)
	{
		Real32 r = context.State.Stack.PopValue<Real32>();
		context.State.Stack.PushValue(sqrt(r));
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
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"plotpixel"), PlotPixel));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"sqrt"), Sqrt));
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
	{
		FunctionSignature signature;
		signature.AddParameter(L"bits", Metadata::EpochType_Buffer, true);
		signature.AddParameter(L"offset", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"color", Metadata::EpochType_Integer, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"plotpixel"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"r", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"sqrt"), signature));
	}
}


void DebugLibrary::LinkToTestHarness(unsigned* harness)
{
	TestHarness = harness;
}

