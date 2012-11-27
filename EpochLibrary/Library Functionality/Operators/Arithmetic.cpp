//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for arithmetic operators
//

#include "pch.h"

#include "Library Functionality/Operators/Arithmetic.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Libraries/Library.h"
#include "Libraries/LibraryJIT.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ActiveScope.h"
#include "Metadata/Precedences.h"



namespace
{
	
	//
	// Sum two numbers and return the result
	//
	void AddIntegers(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 + p2);
	}

	//
	// Subtract two numbers and return the result
	//
	void SubtractIntegers(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 - p2);
	}

	//
	// Multiply two numbers and return the result
	//
	void MultiplyIntegers(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 * p2);
	}

	//
	// Divide two numbers and return the result
	//
	void DivideIntegers(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 / p2);
	}


	//
	// Compute the bitwise negation of an operand
	//
	void BitwiseIntegerNot(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p = context.State.Stack.PopValue<Integer32>();
		context.State.Stack.PushValue(~p);
	}

	//
	// Increment a variable's value by one
	//
	void IncrementInteger(StringHandle functionname, VM::ExecutionContext& context)
	{
		context.State.Stack.PushValue(1);
		AddIntegers(functionname, context);
	}

	//
	// Decrement a variable's value by one
	//
	void DecrementInteger(StringHandle functionname, VM::ExecutionContext& context)
	{
		context.State.Stack.PushValue(1);
		SubtractIntegers(functionname, context);
	}


	//
	// Sum two numbers and return the result
	//
	void AddReals(StringHandle, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 + p2);
	}


	//
	// Subtract two numbers and return the result
	//
	void SubtractReals(StringHandle, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 - p2);
	}

	//
	// Multiply two numbers and return the result
	//
	void MultiplyReals(StringHandle, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 * p2);
	}

	//
	// Divide two numbers and return the result
	//
	void DivideReals(StringHandle, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 / p2);
	}


	//
	// Increment a variable's value by one
	//
	void IncrementReal(StringHandle functionname, VM::ExecutionContext& context)
	{
		context.State.Stack.PushValue(1.0f);
		AddReals(functionname, context);
	}

	//
	// Decrement a variable's value by one
	//
	void DecrementReal(StringHandle functionname, VM::ExecutionContext& context)
	{
		context.State.Stack.PushValue(1.0f);
		SubtractReals(functionname, context);
	}


	//
	// JIT helpers for integer arithmetic
	//
	void AddIntegersJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateAdd(p1, p2);
		context.ValuesOnStack.push(result);
	}

	void MultiplyIntegersJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateMul(p1, p2);
		context.ValuesOnStack.push(result);
	}

	//
	// JIT Helpers for real arithemtic
	//
	void AddRealsJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFAdd(p1, p2);
		context.ValuesOnStack.push(result);
	}

	void SubtractRealsJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFSub(p1, p2);
		context.ValuesOnStack.push(result);
	}

	void MultiplyRealsJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFMul(p1, p2);
		context.ValuesOnStack.push(result);
	}

	void DivideRealsJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFDiv(p1, p2);
		context.ValuesOnStack.push(result);
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

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+=@@integer"), AddIntegers));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-=@@integer"), SubtractIntegers));

	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"+=@@real"), AddReals));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"-=@@real"), SubtractReals));

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
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"*@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"/@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"*@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"/@@real"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+=@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-=@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"+=@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"-=@@real"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"++@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"--@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"++@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
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
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_ADDSUBTRACT));
	}
	{
		StringHandle handle = stringpool.Pool(L"-");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_ADDSUBTRACT));
	}
	{
		StringHandle handle = stringpool.Pool(L"*");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_MULTIPLYDIVIDE));
	}
	{
		StringHandle handle = stringpool.Pool(L"/");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_MULTIPLYDIVIDE));
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

void ArithmeticLibrary::RegisterJITTable(JIT::JITTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"+@@real"), &AddRealsJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"-@@real"), &SubtractRealsJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"*@@real"), &MultiplyRealsJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"/@@real"), &DivideRealsJIT));

	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"+=@@integer"), &AddIntegersJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"*@@integer"), &MultiplyIntegersJIT));
}

