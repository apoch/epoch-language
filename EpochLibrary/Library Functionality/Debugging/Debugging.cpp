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

#include <llvm/IR/Intrinsics.h>


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
	bool PlotPixelJIT(JIT::JITContext& context, bool)
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

		return true;
	}

	// TODO - move this to a better home
	bool SqrtJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* v = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* r = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Intrinsic_Sqrt], v);
		context.ValuesOnStack.push(r);

		return true;
	}

	bool BreakpointJIT(JIT::JITContext& context, bool)
	{
		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_Break]);
		return true;
	}
}

extern "C" void EpochLib_Print(StringHandle strhandle)
{
	UI::OutputStream out;
	out << GlobalExecutionContext->GetPooledString(strhandle) << std::endl;
}

void EpochLib_Assert(bool assumption)
{
	if(!assumption)
	{
		UI::OutputStream output;
		output << UI::lightred << L"Assertion failure" << UI::white << std::endl;

		throw AssertionFailureException();
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
		signature.AddParameter(L"str", Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(PrintHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(ReadHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"value", Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(AssertHandle, signature));
	}
	{
		FunctionSignature signature;
		AddToMapNoDupe(signatureset, std::make_pair(PassTestHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"bits", Metadata::MakeReferenceType(Metadata::EpochType_Buffer));
		signature.AddParameter(L"offset", Metadata::EpochType_Integer);
		signature.AddParameter(L"color", Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(PlotPixelHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"r", Metadata::EpochType_Real);
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

