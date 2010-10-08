//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for comparison operators
//

#include "pch.h"

#include "Library Functionality/Operators/Comparison.h"
#include "Library Functionality/Operators/Precedences.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Libraries/Library.h"

#include "Virtual Machine/VirtualMachine.h"


using namespace ComparisonLibrary;



//
// Bind the library to an execution dispatch table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"=="), ComparisonLibrary::IntegerEquality));
}

//
// Bind the library to a function metadata table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer);
		signature.AddParameter(L"i2", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"=="), signature));
	}
}

//
// Bind the library to the compiler's internal semantic action table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	// Nothing to do for this library
}


//
// Bind the library to the infix operator table
//
void ComparisonLibrary::RegisterInfixOperators(InfixTable& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool)
{
	{
		StringHandle handle = stringpool.Pool(L"==");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, handle));
	}
}


//
// Compare two integers for equality
//
void ComparisonLibrary::IntegerEquality(StringHandle functionname, VM::ExecutionContext& context)
{
	Integer32 p2 = context.State.Stack.PopValue<Integer32>();
	Integer32 p1 = context.State.Stack.PopValue<Integer32>();

	context.State.Stack.PushValue(p1 == p2);
}
