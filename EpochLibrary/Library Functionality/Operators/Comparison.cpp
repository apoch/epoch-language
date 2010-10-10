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



namespace
{
	//
	// Compare two integers for equality
	//
	void IntegerEquality(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 == p2);
	}

	//
	// Compare two integers for inequality
	//
	void IntegerInequality(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 != p2);
	}


	//
	// Compare two integers to see if one is greater than the other
	//
	void IntegerGreaterThan(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 > p2);
	}
}



//
// Bind the library to an execution dispatch table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"=="), IntegerEquality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!="), IntegerInequality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L">"), IntegerGreaterThan));
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
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer);
		signature.AddParameter(L"i2", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!="), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer);
		signature.AddParameter(L"i2", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L">"), signature));
	}
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
	{
		StringHandle handle = stringpool.Pool(L"!=");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, handle));
	}
	{
		StringHandle handle = stringpool.Pool(L">");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, handle));
	}
}


