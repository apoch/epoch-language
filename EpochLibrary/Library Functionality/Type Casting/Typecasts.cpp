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


namespace
{
	void CastIntegerToString(StringHandle functionname, ExecutionContext& context)
	{
		Integer32 value = context.State.Stack.PopValue<Integer32>();
		StringHandle targettype = context.State.Stack.PopValue<StringHandle>();

		std::wostringstream converter;
		converter << value;
		StringHandle result = context.OwnerVM.PoolString(converter.str());

		context.State.Stack.PushValue(result);
	}

	void CastStringToInteger(StringHandle functionname, ExecutionContext& context)
	{
		StringHandle stringhandle = context.State.Stack.PopValue<StringHandle>();
		StringHandle targettype = context.State.Stack.PopValue<StringHandle>();

		std::wstringstream converter(context.OwnerVM.GetPooledString(stringhandle));
		Integer32 result;
		converter >> result;

		context.State.Stack.PushValue(result);
	}

	void CastBooleanToString(StringHandle functionname, ExecutionContext& context)
	{
		bool value = context.State.Stack.PopValue<bool>();
		StringHandle targettype = context.State.Stack.PopValue<StringHandle>();

		StringHandle result;
		if(value)
			result = context.OwnerVM.PoolString(L"true");
		else
			result = context.OwnerVM.PoolString(L"false");

		context.State.Stack.PushValue(result);
	}

	void CastRealToString(StringHandle functionname, ExecutionContext& context)
	{
		Real32 value = context.State.Stack.PopValue<Real32>();
		StringHandle targettype = context.State.Stack.PopValue<StringHandle>();

		std::wostringstream converter;
		converter << value;
		StringHandle result = context.OwnerVM.PoolString(converter.str());

		context.State.Stack.PushValue(result);
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
}

//
// Bind the library to a function metadata table
//
void TypeCasts::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"string"));
		signature.AddParameter(L"value", EpochType_Integer);
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@integer_to_string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"integer"));
		signature.AddParameter(L"value", EpochType_String);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@string_to_integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"string"));
		signature.AddParameter(L"value", EpochType_Boolean);
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@boolean_to_string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"string"));
		signature.AddParameter(L"value", EpochType_Real);
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cast@@real_to_string"), signature));
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
	}
}

