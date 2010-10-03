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


using namespace ArithmeticLibrary;



//
// Bind the library to an execution dispatch table
//
void ArithmeticLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+"), ArithmeticLibrary::AddIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-"), ArithmeticLibrary::SubtractIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"*"), ArithmeticLibrary::MultiplyIntegers));
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
}

//
// Bind the library to the compiler's internal semantic action table
//
void ArithmeticLibrary::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	// Nothing to do for this library
}


//
// Bind the library to the infix operator table
//
void ArithmeticLibrary::RegisterInfixOperators(InfixTable& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool)
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
}



//
// Sum two numbers and return the result
//
void ArithmeticLibrary::AddIntegers(StringHandle functionname, VM::ExecutionContext& context)
{
	Integer32 p2 = context.State.Stack.PopValue<Integer32>();
	Integer32 p1 = context.State.Stack.PopValue<Integer32>();

	context.State.Stack.PushValue(p1 + p2);
}


//
// Subtract two numbers and return the result
//
void ArithmeticLibrary::SubtractIntegers(StringHandle functionname, VM::ExecutionContext& context)
{
	Integer32 p2 = context.State.Stack.PopValue<Integer32>();
	Integer32 p1 = context.State.Stack.PopValue<Integer32>();

	context.State.Stack.PushValue(p1 - p2);
}

//
// Multiply two numbers and return the result
void ArithmeticLibrary::MultiplyIntegers(StringHandle functionname, VM::ExecutionContext& context)
{
	Integer32 p2 = context.State.Stack.PopValue<Integer32>();
	Integer32 p1 = context.State.Stack.PopValue<Integer32>();

	context.State.Stack.PushValue(p1 * p2);
}
