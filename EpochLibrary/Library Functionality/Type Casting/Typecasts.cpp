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

#include <sstream>


using namespace TypeCasts;
using namespace VM;


//
// Bind the library to an execution dispatch table
//
void TypeCasts::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	// TODO - complain on duplicates
	table.insert(std::make_pair(stringpool.Pool(L"cast"), TypeCasts::Cast));
}

//
// Bind the library to a function metadata table
//
void TypeCasts::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	// TODO - complain on duplicates
	{
		FunctionSignature signature;
		signature.AddParameter(L"typename", EpochType_Identifier);
		signature.AddParameter(L"value", EpochType_Integer);				// TODO - overload cast() to support additional parameter types
		signature.SetReturnType(VM::EpochType_String);						// TODO - how do we handle this for non-strings??
		signatureset.insert(std::make_pair(stringpool.Pool(L"cast"), signature));
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
// Convert between primitive types
//
void TypeCasts::Cast(StringHandle functionname, ExecutionContext& context)
{
	Integer32 value = context.State.Stack.PopValue<Integer32>();
	StringHandle targettype = context.State.Stack.PopValue<StringHandle>();

	std::wostringstream converter;
	converter << value;
	StringHandle result = context.OwnerVM.PoolString(converter.str());

	context.State.Stack.PushValue(result);
}

