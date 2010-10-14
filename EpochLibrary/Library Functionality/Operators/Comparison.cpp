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
	// Compare two booleans for equality
	//
	void BooleanEquality(StringHandle functionname, VM::ExecutionContext& context)
	{
		bool p2 = context.State.Stack.PopValue<bool>();
		bool p1 = context.State.Stack.PopValue<bool>();

		context.State.Stack.PushValue(p1 == p2);
	}

	//
	// Compare two booleans for inequality
	//
	void BooleanInequality(StringHandle functionname, VM::ExecutionContext& context)
	{
		bool p2 = context.State.Stack.PopValue<bool>();
		bool p1 = context.State.Stack.PopValue<bool>();

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

	//
	// Compare two integers to see if one is less than the other
	//
	void IntegerLessThan(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 < p2);
	}


	//
	// Compare two reals to see if one is greater than the other
	//
	void RealGreaterThan(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 > p2);
	}

	//
	// Compare two reals to see if one is less than the other
	//
	void RealLessThan(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 < p2);
	}
}



//
// Bind the library to an execution dispatch table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"==@@integer"), IntegerEquality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!=@@integer"), IntegerInequality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"==@@boolean"), BooleanEquality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!=@@boolean"), BooleanInequality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L">@@integer"), IntegerGreaterThan));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"<@@integer"), IntegerLessThan));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L">@@real"), RealGreaterThan));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"<@@real"), RealLessThan));
}

//
// Bind the library to a function metadata table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"==@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!=@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Boolean, false);
		signature.AddParameter(L"i2", VM::EpochType_Boolean, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"==@@boolean"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Boolean, false);
		signature.AddParameter(L"i2", VM::EpochType_Boolean, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!=@@boolean"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L">@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"<@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Real, false);
		signature.AddParameter(L"i2", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L">@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Real, false);
		signature.AddParameter(L"i2", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"<@@real"), signature));
	}
}


//
// Bind the library to the infix operator table
//
void ComparisonLibrary::RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool)
{
	{
		StringHandle handle = stringpool.Pool(L"==");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L"==@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L"==@@boolean")));
	}
	{
		StringHandle handle = stringpool.Pool(L"!=");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L"!=@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L"!=@@boolean")));
	}
	{
		StringHandle handle = stringpool.Pool(L">");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L">@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L">@@real")));
	}
	{
		StringHandle handle = stringpool.Pool(L"<");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L"<@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_COMPARISON, stringpool.Pool(L"<@@real")));
	}
}


//
// Register the list of overloads used by functions in this library module
//
void ComparisonLibrary::RegisterLibraryOverloads(OverloadMap& overloadmap, StringPoolManager& stringpool)
{
	{
		StringHandle functionnamehandle = stringpool.Pool(L"==");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"==@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"==@@boolean"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"!=");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!=@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!=@@boolean"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L">");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L">@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L">@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"<");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"<@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"<@@real"));
	}
}