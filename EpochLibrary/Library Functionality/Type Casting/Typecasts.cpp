//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for converting data between types
//

#include "pch.h"

#include "Library Functionality/Type Casting/Typecasts.h"

#include "Runtime/Runtime.h"

#include "Utility/Types/RealTypes.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include <sstream>


using namespace Runtime;
using namespace Metadata;

extern Runtime::ExecutionContext* GlobalExecutionContext;


namespace
{

	StringHandle StringTypeHandle = 0;
	StringHandle IntegerTypeHandle = 0;
	StringHandle RealTypeHandle = 0;

	StringHandle CastIntegerToStringHandle = 0;
	StringHandle CastStringToIntegerHandle = 0;

	StringHandle CastIntegerToRealHandle = 0;
	StringHandle CastRealToIntegerHandle = 0;

	StringHandle CastBooleanToStringHandle = 0;
	StringHandle CastRealToStringHandle = 0;
	StringHandle CastBufferToStringHandle = 0;

	StringHandle CastBooleanToIntegerHandle = 0;


	bool CastRealToIntegerJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		// Pop "integer" token
		context.ValuesOnStack.pop();

		llvm::LLVMContext& llvmcontext = *reinterpret_cast<llvm::LLVMContext*>(context.Context);
		llvm::Value* castvalue = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCast(llvm::Instruction::FPToSI, p, llvm::Type::getInt32Ty(llvmcontext));
		context.ValuesOnStack.push(castvalue);

		return true;
	}

	bool CastIntegerToRealJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		// Pop "real" token
		context.ValuesOnStack.pop();

		llvm::LLVMContext& llvmcontext = *reinterpret_cast<llvm::LLVMContext*>(context.Context);
		llvm::Value* castvalue = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCast(llvm::Instruction::SIToFP, p, llvm::Type::getFloatTy(llvmcontext));
		context.ValuesOnStack.push(castvalue);

		return true;
	}

	bool CastBooleanToIntegerJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		// Pop "integer" token
		context.ValuesOnStack.pop();

		llvm::LLVMContext& llvmcontext = *reinterpret_cast<llvm::LLVMContext*>(context.Context);
		llvm::Value* castvalue = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCast(llvm::Instruction::ZExt, p, llvm::Type::getInt32Ty(llvmcontext));
		context.ValuesOnStack.push(castvalue);

		return true;
	}
}


//
// Bind the library to a function metadata table
//
void TypeCasts::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(StringTypeHandle);
		signature.AddParameter(L"value", EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastIntegerToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(IntegerTypeHandle);
		signature.AddParameter(L"value", EpochType_String, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CastStringToIntegerHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(StringTypeHandle);
		signature.AddParameter(L"value", EpochType_Boolean, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastBooleanToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(StringTypeHandle);
		signature.AddParameter(L"value", EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastRealToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(StringTypeHandle);
		signature.AddParameter(L"value", EpochType_Buffer, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastBufferToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(IntegerTypeHandle);
		signature.AddParameter(L"value", EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CastRealToIntegerHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(RealTypeHandle);
		signature.AddParameter(L"value", EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(CastIntegerToRealHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(IntegerTypeHandle);
		signature.AddParameter(L"value", EpochType_Boolean, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CastBooleanToIntegerHandle, signature));
	}
}


//
// Register the list of overloads used by functions in this library module
//
void TypeCasts::RegisterLibraryOverloads(OverloadMap& overloadmap, StringPoolManager& stringpool)
{
	{
		StringHandle functionnamehandle = stringpool.Pool(L"cast");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@integer_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@string_to_integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@boolean_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@real_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@buffer_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@real_to_integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@integer_to_real"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@boolean_to_integer"));
	}
}

void TypeCasts::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastRealToIntegerHandle, &CastRealToIntegerJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastIntegerToRealHandle, &CastIntegerToRealJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastBooleanToIntegerHandle, &CastBooleanToIntegerJIT));

	AddToMapNoDupe(table.LibraryExports, std::make_pair(CastRealToStringHandle, "EpochLib_CastRealToStr"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(CastBufferToStringHandle, "EpochLib_CastBufferToStr"));
}


void TypeCasts::PoolStrings(StringPoolManager& stringpool)
{
	StringTypeHandle = stringpool.Pool(L"string");
	IntegerTypeHandle = stringpool.Pool(L"integer");
	RealTypeHandle = stringpool.Pool(L"real");

	CastIntegerToStringHandle = stringpool.Pool(L"cast@@integer_to_string");
	CastStringToIntegerHandle = stringpool.Pool(L"cast@@string_to_integer");

	CastRealToIntegerHandle = stringpool.Pool(L"cast@@real_to_integer");
	CastIntegerToRealHandle = stringpool.Pool(L"cast@@integer_to_real");

	CastBooleanToStringHandle = stringpool.Pool(L"cast@@boolean_to_string");
	CastRealToStringHandle = stringpool.Pool(L"cast@@real_to_string");
	CastBufferToStringHandle = stringpool.Pool(L"cast@@buffer_to_string");

	CastBooleanToIntegerHandle = stringpool.Pool(L"cast@@boolean_to_integer");
}



extern "C" StringHandle EpochLib_CastRealToStr(float real)
{
	std::wostringstream convert;
	convert << real;

	return GlobalExecutionContext->PoolString(convert.str());
}

extern "C" StringHandle EpochLib_CastBufferToStr(BufferHandle buffer)
{
	std::wstring str(reinterpret_cast<wchar_t*>(GlobalExecutionContext->GetBuffer(buffer)), GlobalExecutionContext->GetBufferSize(buffer));
	StringHandle result = GlobalExecutionContext->PoolString(str);
	GlobalExecutionContext->TickStringGarbageCollector();
	return result;
}

