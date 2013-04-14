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
#include "Utility/DllPool.h"

#include "User Interface/Output.h"
#include "User Interface/Input.h"

#include "Runtime/Runtime.h"

#include <llvm/Intrinsics.h>


extern Runtime::ExecutionContext* GlobalExecutionContext;


namespace
{

	unsigned* TestHarness = NULL;


	StringHandle PrintHandle = 0;
	StringHandle ReadHandle = 0;
	StringHandle AssertHandle = 0;
	StringHandle PassTestHandle = 0;
	StringHandle SqrtHandle = 0;
	StringHandle PlotPixelHandle = 0;
	StringHandle BreakpointHandle = 0;


	// TODO - lame hack; replace with real array support
	void PlotPixelJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* color = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* offset = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* pbufferhandle = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* bufferhandle = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateLoad(pbufferhandle);

		llvm::Value* cachedcall = context.BufferLookupCache[pbufferhandle];
		if(!cachedcall)
		{
			cachedcall = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_GetBuffer], bufferhandle);
			context.BufferLookupCache[pbufferhandle] = cachedcall;
		}

		llvm::Value* bufferptr = cachedcall;
		llvm::Value* castbufferptr = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreatePointerCast(bufferptr, llvm::Type::getInt32PtrTy(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));

		llvm::Value* gep = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateGEP(castbufferptr, offset);
		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateStore(color, gep);
	}

	// TODO - move this to a better home
	void SqrtJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* v = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* r = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Intrinsic_Sqrt], v);
		context.ValuesOnStack.push(r);
	}

	void BreakpointJIT(JIT::JITContext& context, bool)
	{
		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_Break]);
	}
}

extern "C" void EpochLib_Print(StringHandle strhandle)
{
	UI::OutputStream out;
	out << GlobalExecutionContext->GetPooledString(strhandle) << std::endl;
}

extern "C" void EpochLib_Assert(bool assumption)
{
	if(!assumption)
	{
		UI::OutputStream output;
		output << UI::lightred << L"Assertion failure" << UI::white << std::endl;

		typedef void (STDCALL *haltfunc)();
		Marshaling::DLLPool::DLLPoolHandle handle = Marshaling::TheDLLPool.OpenDLL(L"EpochRuntime.dll");
		Marshaling::TheDLLPool.GetFunction<haltfunc>(handle, "Epoch_Halt")();
	}
}

extern "C" void EpochLib_PassTest()
{
	UI::OutputStream stream;
	stream << UI::lightblue << L"TEST: " << UI::resetcolor << L"Pass" << std::endl;

	if(TestHarness)
		++(*TestHarness);
}


//
// Bind the library to a function metadata table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String, false);
		AddToMapNoDupe(signatureset, std::make_pair(PrintHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(ReadHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"value", Metadata::EpochType_Boolean, false);
		AddToMapNoDupe(signatureset, std::make_pair(AssertHandle, signature));
	}
	{
		FunctionSignature signature;
		AddToMapNoDupe(signatureset, std::make_pair(PassTestHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"bits", Metadata::EpochType_Buffer, true);
		signature.AddParameter(L"offset", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"color", Metadata::EpochType_Integer, false);
		AddToMapNoDupe(signatureset, std::make_pair(PlotPixelHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"r", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(SqrtHandle, signature));
	}
	{
		FunctionSignature signature;
		AddToMapNoDupe(signatureset, std::make_pair(BreakpointHandle, signature));
	}
}


void DebugLibrary::LinkToTestHarness(unsigned* harness)
{
	TestHarness = harness;
}


void DebugLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.LibraryExports, std::make_pair(AssertHandle, "EpochLib_Assert"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(PassTestHandle, "EpochLib_PassTest"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(PrintHandle, "EpochLib_Print"));

	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(SqrtHandle, &SqrtJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(PlotPixelHandle, &PlotPixelJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(BreakpointHandle, &BreakpointJIT));
}


void DebugLibrary::PoolStrings(StringPoolManager& stringpool)
{
	PrintHandle = stringpool.Pool(L"print");
	ReadHandle = stringpool.Pool(L"read");
	AssertHandle = stringpool.Pool(L"assert");
	PassTestHandle = stringpool.Pool(L"passtest");
	SqrtHandle = stringpool.Pool(L"sqrt");
	PlotPixelHandle = stringpool.Pool(L"plotpixel");
	BreakpointHandle = stringpool.Pool(L"breakpoint");
}

