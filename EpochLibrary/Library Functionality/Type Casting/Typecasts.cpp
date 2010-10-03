//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for converting data between types
//

#include "pch.h"

#include "Library Functionality/Type Casting/Typecasts.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include <sstream>


using namespace TypeCasts;
using namespace VM;


//
// Bind the library to an execution dispatch table
//
void TypeCasts::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@integer_to_string"), TypeCasts::CastIntegerToString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cast@@string_to_integer"), TypeCasts::CastStringToInteger));
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
}

//
// Bind the library to the compiler's internal semantic action table
//
void TypeCasts::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	// Nothing to do for this library
}


//
// Register the list of overloads used by functions in this library module
//
void TypeCasts::RegisterLibraryOverloads(std::map<StringHandle, std::set<StringHandle> >& overloadmap, StringPoolManager& stringpool)
{
	{
		StringHandle functionnamehandle = stringpool.Pool(L"cast");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@integer_to_string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"cast@@string_to_integer"));
	}
}

//
// Convert between primitive types
//
void TypeCasts::CastIntegerToString(StringHandle functionname, ExecutionContext& context)
{
	Integer32 value = context.State.Stack.PopValue<Integer32>();
	StringHandle targettype = context.State.Stack.PopValue<StringHandle>();

	std::wostringstream converter;
	converter << value;
	StringHandle result = context.OwnerVM.PoolString(converter.str());

	context.State.Stack.PushValue(result);
}

void TypeCasts::CastStringToInteger(StringHandle functionname, ExecutionContext& context)
{
	StringHandle stringhandle = context.State.Stack.PopValue<StringHandle>();
	StringHandle targettype = context.State.Stack.PopValue<StringHandle>();

	std::wstringstream converter(context.OwnerVM.GetPooledString(stringhandle));
	Integer32 result;
	converter >> result;

	context.State.Stack.PushValue(result);
}
