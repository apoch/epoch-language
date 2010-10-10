//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for arithmetic operators
//

#include "pch.h"

#include "Library Functionality/Operators/Arithmetic.h"
#include "Library Functionality/Operators/Precedences.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Libraries/Library.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Metadata/ActiveScope.h"



namespace
{
	
	//
	// Sum two numbers and return the result
	//
	void AddIntegers(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 + p2);
	}


	//
	// Subtract two numbers and return the result
	//
	void SubtractIntegers(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 - p2);
	}

	//
	// Multiply two numbers and return the result
	//
	void MultiplyIntegers(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 * p2);
	}

	//
	// Divide two numbers and return the result
	//
	void DivideIntegers(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 / p2);
	}


	//
	// Compute the bitwise negation of an operand
	//
	void BitwiseIntegerNot(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 p = context.State.Stack.PopValue<Integer32>();
		context.State.Stack.PushValue(~p);
	}


	//
	// Read a variable's value, add a value to it, then return the result
	//
	void AddAssign(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 value = context.State.Stack.PopValue<Integer32>();
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(value);
		AddIntegers(functionname, context);
	}

	//
	// Read a variable's value, subtract a value from it, then return the result
	//
	void SubtractAssign(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 value = context.State.Stack.PopValue<Integer32>();
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(value);
		SubtractIntegers(functionname, context);
	}

}



//
// Bind the library to an execution dispatch table
//
void ArithmeticLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+"), AddIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-"), SubtractIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"*"), MultiplyIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"/"), DivideIntegers));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!@@integer"), BitwiseIntegerNot));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+="), AddAssign));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-="), SubtractAssign));
}

//
// Bind the library to a function metadata table
//
void ArithmeticLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer);
		signature.AddParameter(L"i2", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer);
		signature.AddParameter(L"i2", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer);
		signature.AddParameter(L"i2", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"*"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer);
		signature.AddParameter(L"i2", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"/"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier);
		signature.AddParameter(L"i", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+="), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier);
		signature.AddParameter(L"i", VM::EpochType_Integer);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-="), signature));
	}
}

//
// Bind the library to the infix operator table
//
void ArithmeticLibrary::RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool)
{
	{
		StringHandle handle = stringpool.Pool(L"+");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_ADDSUBTRACT, handle));
	}
	{
		StringHandle handle = stringpool.Pool(L"-");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_ADDSUBTRACT, handle));
	}
	{
		StringHandle handle = stringpool.Pool(L"*");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_MULTIPLYDIVIDE, handle));
	}
	{
		StringHandle handle = stringpool.Pool(L"/");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_MULTIPLYDIVIDE, handle));
	}
}

//
// Register the list of unary operators provided by the library module
//
void ArithmeticLibrary::RegisterUnaryOperators(std::set<std::wstring>& unaryprefixes, StringPoolManager& stringpool)
{
	{
		StringHandle handle = stringpool.Pool(L"!");
		AddToSetNoDupe(unaryprefixes, stringpool.GetPooledString(handle));
	}
}

//
// Register the list of overloads used by functions in this library module
//
void ArithmeticLibrary::RegisterLibraryOverloads(std::map<StringHandle, std::set<StringHandle> >& overloadmap, StringPoolManager& stringpool)
{
	{
		StringHandle functionnamehandle = stringpool.Pool(L"!");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!@@integer"));
	}
}

//
// Register the list of operation-assignment operators provided by this library module
//
void ArithmeticLibrary::RegisterOpAssignOperators(StringSet& operators, StringPoolManager& stringpool)
{
	{
		StringHandle handle = stringpool.Pool(L"+=");
		AddToSetNoDupe(operators, stringpool.GetPooledString(handle));
	}
	{
		StringHandle handle = stringpool.Pool(L"-=");
		AddToSetNoDupe(operators, stringpool.GetPooledString(handle));
	}
}

