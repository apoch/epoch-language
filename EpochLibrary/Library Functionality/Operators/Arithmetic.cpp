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
	void AddAssignInteger(StringHandle functionname, VM::ExecutionContext& context)
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
	void SubtractAssignInteger(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 value = context.State.Stack.PopValue<Integer32>();
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(value);
		SubtractIntegers(functionname, context);
	}


	//
	// Increment a variable's value by one
	//
	void IncrementInteger(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(1);
		AddIntegers(functionname, context);
	}

	//
	// Decrement a variable's value by one
	//
	void DecrementInteger(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(1);
		SubtractIntegers(functionname, context);
	}


	//
	// Sum two numbers and return the result
	//
	void AddReals(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 + p2);
	}


	//
	// Subtract two numbers and return the result
	//
	void SubtractReals(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 - p2);
	}

	//
	// Multiply two numbers and return the result
	//
	void MultiplyReals(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 * p2);
	}

	//
	// Divide two numbers and return the result
	//
	void DivideReals(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 / p2);
	}


	//
	// Read a variable's value, add a value to it, then return the result
	//
	void AddAssignReal(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 value = context.State.Stack.PopValue<Real32>();
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(value);
		AddReals(functionname, context);
	}

	//
	// Read a variable's value, subtract a value from it, then return the result
	//
	void SubtractAssignReal(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 value = context.State.Stack.PopValue<Real32>();
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(value);
		SubtractReals(functionname, context);
	}


	//
	// Increment a variable's value by one
	//
	void IncrementReal(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(1);
		AddReals(functionname, context);
	}

	//
	// Decrement a variable's value by one
	//
	void DecrementReal(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		context.Variables->PushOntoStack(identifier, context.State.Stack);
		context.State.Stack.PushValue(1);
		SubtractReals(functionname, context);
	}
}



//
// Bind the library to an execution dispatch table
//
void ArithmeticLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+@@integer"), AddIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-@@integer"), SubtractIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"*@@integer"), MultiplyIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"/@@integer"), DivideIntegers));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+@@real"), AddReals));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-@@real"), SubtractReals));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"*@@real"), MultiplyReals));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"/@@real"), DivideReals));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!@@integer"), BitwiseIntegerNot));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+=@@integer"), AddAssignInteger));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-=@@integer"), SubtractAssignInteger));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+=@@real"), AddAssignReal));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-=@@real"), SubtractAssignReal));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"++@@integer"), IncrementInteger));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"--@@integer"), DecrementInteger));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"++@@real"), IncrementReal));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"--@@real"), DecrementReal));
}

//
// Bind the library to a function metadata table
//
void ArithmeticLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"*@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Integer, false);
		signature.AddParameter(L"i2", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"/@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Real, false);
		signature.AddParameter(L"i2", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Real, false);
		signature.AddParameter(L"i2", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Real, false);
		signature.AddParameter(L"i2", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"*@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", VM::EpochType_Real, false);
		signature.AddParameter(L"i2", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"/@@real"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"i", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+=@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"i", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-=@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"i", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+=@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"i", VM::EpochType_Real, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-=@@real"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"++@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"--@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"++@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.SetReturnType(VM::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"--@@real"), signature));
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
		precedences.insert(std::make_pair(PRECEDENCE_ADDSUBTRACT, stringpool.Pool(L"+@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_ADDSUBTRACT, stringpool.Pool(L"+@@real")));
	}
	{
		StringHandle handle = stringpool.Pool(L"-");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_ADDSUBTRACT, stringpool.Pool(L"-@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_ADDSUBTRACT, stringpool.Pool(L"-@@real")));
	}
	{
		StringHandle handle = stringpool.Pool(L"*");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_MULTIPLYDIVIDE, stringpool.Pool(L"*@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_MULTIPLYDIVIDE, stringpool.Pool(L"*@@real")));
	}
	{
		StringHandle handle = stringpool.Pool(L"/");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		precedences.insert(std::make_pair(PRECEDENCE_MULTIPLYDIVIDE, stringpool.Pool(L"/@@integer")));
		precedences.insert(std::make_pair(PRECEDENCE_MULTIPLYDIVIDE, stringpool.Pool(L"/@@real")));
	}
}

//
// Register the list of unary operators provided by the library module
//
void ArithmeticLibrary::RegisterUnaryOperators(StringSet& unaryprefixes, StringSet& preoperators, StringSet& postoperators, StringPoolManager& stringpool)
{
	{
		StringHandle handle = stringpool.Pool(L"!");
		AddToSetNoDupe(unaryprefixes, stringpool.GetPooledString(handle));
	}

	{
		StringHandle handle = stringpool.Pool(L"++");
		AddToSetNoDupe(preoperators, stringpool.GetPooledString(handle));
		AddToSetNoDupe(postoperators, stringpool.GetPooledString(handle));
	}
	{
		StringHandle handle = stringpool.Pool(L"--");
		AddToSetNoDupe(preoperators, stringpool.GetPooledString(handle));
		AddToSetNoDupe(postoperators, stringpool.GetPooledString(handle));
	}
}

//
// Register the list of overloads used by functions in this library module
//
void ArithmeticLibrary::RegisterLibraryOverloads(OverloadMap& overloadmap, StringPoolManager& stringpool)
{
	{
		StringHandle functionnamehandle = stringpool.Pool(L"!");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!@@integer"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"+");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"+@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"+@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"-");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"-@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"-@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"*");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"*@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"*@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"/");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"/@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"/@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"+=");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"+=@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"+=@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"-=");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"-=@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"-=@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"++");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"++@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"++@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"--");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"--@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"--@@real"));
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

