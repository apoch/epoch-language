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
	StringHandle Integer16TypeHandle = 0;
	StringHandle RealTypeHandle = 0;

	StringHandle CastIntegerToStringHandle = 0;
	StringHandle CastStringToIntegerHandle = 0;

	StringHandle CastIntegerToRealHandle = 0;
	StringHandle CastRealToIntegerHandle = 0;

	StringHandle CastBooleanToStringHandle = 0;
	StringHandle CastRealToStringHandle = 0;
	StringHandle CastBufferToStringHandle = 0;

	StringHandle CastBooleanToIntegerHandle = 0;

	StringHandle CastIntegerToInteger16Handle = 0;
	StringHandle CastInteger16ToIntegerHandle = 0;


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

	bool CastIntegerToInteger16JIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		// Pop "integer16" token
		context.ValuesOnStack.pop();

		llvm::LLVMContext& llvmcontext = *reinterpret_cast<llvm::LLVMContext*>(context.Context);
		llvm::Value* castvalue = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCast(llvm::Instruction::Trunc, p, llvm::Type::getInt16Ty(llvmcontext));
		context.ValuesOnStack.push(castvalue);

		return true;
	}

	bool CastInteger16ToIntegerJIT(JIT::JITContext& context, bool)
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
		signature.AddParameter(L"value", EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastIntegerToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(IntegerTypeHandle);
		signature.AddParameter(L"value", EpochType_String);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CastStringToIntegerHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(StringTypeHandle);
		signature.AddParameter(L"value", EpochType_Boolean);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastBooleanToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(StringTypeHandle);
		signature.AddParameter(L"value", EpochType_Real);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastRealToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(StringTypeHandle);
		signature.AddParameter(L"value", Metadata::MakeReferenceType(EpochType_Buffer));
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CastBufferToStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(IntegerTypeHandle);
		signature.AddParameter(L"value", EpochType_Real);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CastRealToIntegerHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(RealTypeHandle);
		signature.AddParameter(L"value", EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(CastIntegerToRealHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(IntegerTypeHandle);
		signature.AddParameter(L"value", EpochType_Boolean);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CastBooleanToIntegerHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(Integer16TypeHandle);
		signature.AddParameter(L"value", EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_Integer16);
		AddToMapNoDupe(signatureset, std::make_pair(CastIntegerToInteger16Handle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(IntegerTypeHandle);
		signature.AddParameter(L"value", EpochType_Integer16);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CastInteger16ToIntegerHandle, signature));
	}
}


//
// Register the list of overloads used by functions in this library module
//
void TypeCasts::RegisterLibraryOverloads(OverloadMap& overloadmap, StringPoolManager& stringpool)
{
	{
		// TODO - remove pool calls
		StringHandle functionnamehandle = stringpool.Pool(L"cast");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@integer_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@string_to_integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@boolean_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@real_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@buffer_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@real_to_integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@integer_to_real"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@boolean_to_integer"));
		overloadmap[functionnamehandle].insert(CastIntegerToInteger16Handle);
		overloadmap[functionnamehandle].insert(CastInteger16ToIntegerHandle);
	}
}

void TypeCasts::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastRealToIntegerHandle, &CastRealToIntegerJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastIntegerToRealHandle, &CastIntegerToRealJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastBooleanToIntegerHandle, &CastBooleanToIntegerJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastIntegerToInteger16Handle, &CastIntegerToInteger16JIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(CastInteger16ToIntegerHandle, &CastInteger16ToIntegerJIT));

	AddToMapNoDupe(table.LibraryExports, std::make_pair(CastIntegerToStringHandle, "EpochLib_CastIntegerToStr"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(CastStringToIntegerHandle, "EpochLib_CastStrToInteger"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(CastRealToStringHandle, "EpochLib_CastRealToStr"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(CastBufferToStringHandle, "EpochLib_CastBufferToStr"));
}


void TypeCasts::PoolStrings(StringPoolManager& stringpool)
{
	StringTypeHandle = stringpool.Pool(L"string");
	IntegerTypeHandle = stringpool.Pool(L"integer");
	Integer16TypeHandle = stringpool.Pool(L"integer16");
	RealTypeHandle = stringpool.Pool(L"real");

	CastIntegerToStringHandle = stringpool.Pool(L"cast@@integer_to_string");
	CastStringToIntegerHandle = stringpool.Pool(L"cast@@string_to_integer");

	CastRealToIntegerHandle = stringpool.Pool(L"cast@@real_to_integer");
	CastIntegerToRealHandle = stringpool.Pool(L"cast@@integer_to_real");

	CastBooleanToStringHandle = stringpool.Pool(L"cast@@boolean_to_string");
	CastRealToStringHandle = stringpool.Pool(L"cast@@real_to_string");
	CastBufferToStringHandle = stringpool.Pool(L"cast@@buffer_to_string");

	CastBooleanToIntegerHandle = stringpool.Pool(L"cast@@boolean_to_integer");

	CastIntegerToInteger16Handle = stringpool.Pool(L"cast@@integer_to_integer16");
	CastInteger16ToIntegerHandle = stringpool.Pool(L"cast@@integer16_to_integer");
}



extern "C" StringHandle EpochLib_CastIntegerToStr(int val)
{
	std::wostringstream convert;
	convert << val;

	return GlobalExecutionContext->PoolString(convert.str());
}

extern "C" int EpochLib_CastStrToInteger(StringHandle h)
{
	int ret = 0;

	std::wstringstream convert;
	convert << GlobalExecutionContext->GetPooledString(h);
	convert >> ret;

	return ret;
}

extern "C" StringHandle EpochLib_CastRealToStr(float real)
{
	std::wostringstream convert;
	convert << real;

	return GlobalExecutionContext->PoolString(convert.str());
}

extern "C" StringHandle EpochLib_CastBufferToStr(BufferHandle* buffer)
{
	std::wstring str(reinterpret_cast<wchar_t*>(GlobalExecutionContext->GetBuffer(*buffer)), wcslen(reinterpret_cast<const wchar_t*>(GlobalExecutionContext->GetBuffer(*buffer))));
	StringHandle result = GlobalExecutionContext->PoolString(str);
	return result;
}

