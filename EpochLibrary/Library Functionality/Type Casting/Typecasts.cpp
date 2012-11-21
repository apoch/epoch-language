//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for converting data between types
//

#include "pch.h"

#include "Library Functionality/Type Casting/Typecasts.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Utility/Types/RealTypes.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include <sstream>


using namespace VM;
using namespace Metadata;


namespace
{
	//
	// Cast an integer value into a string
	//
	// Should tick the garbage collector, since it allocates a new string.
	//
	void CastIntegerToString(StringHandle, ExecutionContext& context)
	{
		Integer32 value = context.State.Stack.PopValue<Integer32>();
		context.State.Stack.PopValue<StringHandle>();

		std::wostringstream converter;
		converter << value;
		StringHandle result = context.OwnerVM.PoolString(converter.str());

		context.State.Stack.PushValue(result);
		context.TickStringGarbageCollector();
	}

	//
	// Parse a string, expecting an integer value represented therein, and return the result
	//
	void CastStringToInteger(StringHandle, ExecutionContext& context)
	{
		StringHandle stringhandle = context.State.Stack.PopValue<StringHandle>();
		context.State.Stack.PopValue<StringHandle>();

		std::wstringstream converter(context.OwnerVM.GetPooledString(stringhandle));
		Integer32 result = 0;
		converter >> result;

		// TODO - runtime conversion exception on failure?

		context.State.Stack.PushValue(result);
	}

	//
	// Convert a boolean value into a string representation ("true" or "false")
	//
	// Ticks over the garbage collector, since it allocates a new string for the result
	//
	void CastBooleanToString(StringHandle, ExecutionContext& context)
	{
		bool value = context.State.Stack.PopValue<bool>();
		context.State.Stack.PopValue<StringHandle>();

		StringHandle result;
		if(value)
			result = context.OwnerVM.PoolString(L"true");
		else
			result = context.OwnerVM.PoolString(L"false");

		context.State.Stack.PushValue(result);
		context.TickStringGarbageCollector();
	}

	//
	// Convert a real number to string format
	//
	// Ticks over the garbage collector.
	//
	void CastRealToString(StringHandle, ExecutionContext& context)
	{
		Real32 value = context.State.Stack.PopValue<Real32>();
		context.State.Stack.PopValue<StringHandle>();

		std::wostringstream converter;
		converter << value;
		StringHandle result = context.OwnerVM.PoolString(converter.str());

		context.State.Stack.PushValue(result);
		context.TickStringGarbageCollector();
	}

	//
	// Convert a byte buffer into a string, assuming the byte buffer represents
	// a sequence of bytes that directly maps to a string value!
	//
	// Ticks over the garbage collector.
	//
	void CastBufferToString(StringHandle, ExecutionContext& context)
	{
		BufferHandle bufferhandle = context.State.Stack.PopValue<BufferHandle>();
		context.State.Stack.PopValue<StringHandle>();

		std::wstring str(reinterpret_cast<wchar_t*>(context.OwnerVM.GetBuffer(bufferhandle)), context.OwnerVM.GetBufferSize(bufferhandle));
		StringHandle result = context.OwnerVM.PoolString(str);

		context.State.Stack.PushValue(result);
		context.TickStringGarbageCollector();
	}


	void CastRealToInteger(StringHandle, ExecutionContext& context)
	{
		Real32 value = context.State.Stack.PopValue<Real32>();
		context.State.Stack.PopValue<StringHandle>();

		context.State.Stack.PushValue(static_cast<Integer32>(value));
	}
}


//
// Bind the library to an execution dispatch table
//
void TypeCasts::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@integer_to_string"), CastIntegerToString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@string_to_integer"), CastStringToInteger));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@boolean_to_string"), CastBooleanToString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@real_to_string"), CastRealToString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@buffer_to_string"), CastBufferToString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@real_to_integer"), CastRealToInteger));
}

//
// Bind the library to a function metadata table
//
void TypeCasts::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"string"));
		signature.AddParameter(L"value", EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@integer_to_string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"integer"));
		signature.AddParameter(L"value", EpochType_String, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@string_to_integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"string"));
		signature.AddParameter(L"value", EpochType_Boolean, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@boolean_to_string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"string"));
		signature.AddParameter(L"value", EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@real_to_string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"string"));
		signature.AddParameter(L"value", EpochType_Buffer, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@buffer_to_string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"integer"));
		signature.AddParameter(L"value", EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@real_to_integer"), signature));
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
	}
}

