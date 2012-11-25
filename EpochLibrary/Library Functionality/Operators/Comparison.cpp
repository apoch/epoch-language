//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for comparison operators
//

#include "pch.h"

#include "Library Functionality/Operators/Comparison.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Libraries/Library.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/Precedences.h"


namespace
{
	//
	// Compare two integers for equality
	//
	void IntegerEquality(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 == p2);
	}

	void Integer16Equality(StringHandle, VM::ExecutionContext& context)
	{
		Integer16 p2 = context.State.Stack.PopValue<Integer16>();
		Integer16 p1 = context.State.Stack.PopValue<Integer16>();

		context.State.Stack.PushValue(p1 == p2);
	}


	//
	// Compare two integers for inequality
	//
	void IntegerInequality(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 != p2);
	}


	//
	// Compare two booleans for equality
	//
	void BooleanEquality(StringHandle, VM::ExecutionContext& context)
	{
		bool p2 = context.State.Stack.PopValue<bool>();
		bool p1 = context.State.Stack.PopValue<bool>();

		context.State.Stack.PushValue(p1 == p2);
	}

	//
	// Compare two booleans for inequality
	//
	void BooleanInequality(StringHandle, VM::ExecutionContext& context)
	{
		bool p2 = context.State.Stack.PopValue<bool>();
		bool p1 = context.State.Stack.PopValue<bool>();

		context.State.Stack.PushValue(p1 != p2);
	}


	//
	// Compare two integers to see if one is greater than the other
	//
	void IntegerGreaterThan(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 > p2);
	}

	//
	// Compare two integers to see if one is less than the other
	//
	void IntegerLessThan(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 p2 = context.State.Stack.PopValue<Integer32>();
		Integer32 p1 = context.State.Stack.PopValue<Integer32>();

		context.State.Stack.PushValue(p1 < p2);
	}


	void IntegerLessThanJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateICmp(llvm::CmpInst::ICMP_SLT, p1, p2);
		context.ValuesOnStack.push(flag);
	}


	void RealEquality(StringHandle, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 == p2);
	}


	//
	// Compare two reals to see if one is greater than the other
	//
	void RealGreaterThan(StringHandle, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 > p2);
	}

	void RealGreaterThanJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFCmp(llvm::CmpInst::FCMP_OGT, p1, p2);
		context.ValuesOnStack.push(flag);
	}

	//
	// Compare two reals to see if one is less than the other
	//
	void RealLessThan(StringHandle, VM::ExecutionContext& context)
	{
		Real32 p2 = context.State.Stack.PopValue<Real32>();
		Real32 p1 = context.State.Stack.PopValue<Real32>();

		context.State.Stack.PushValue(p1 < p2);
	}

	void RealLessThanJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFCmp(llvm::CmpInst::FCMP_OLT, p1, p2);
		context.ValuesOnStack.push(flag);
	}


	//
	// Compare two strings for equality
	//
	void StringEquality(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle p2 = context.State.Stack.PopValue<StringHandle>();
		StringHandle p1 = context.State.Stack.PopValue<StringHandle>();

		context.State.Stack.PushValue((p1 == p2) || (context.OwnerVM.GetPooledString(p1) == context.OwnerVM.GetPooledString(p2)));
	}

	//
	// Compare two strings for inequality
	//
	void StringInequality(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle p2 = context.State.Stack.PopValue<StringHandle>();
		StringHandle p1 = context.State.Stack.PopValue<StringHandle>();

		context.State.Stack.PushValue((p1 != p2) && (context.OwnerVM.GetPooledString(p1) != context.OwnerVM.GetPooledString(p2)));
	}
}



//
// Bind the library to an execution dispatch table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"==@@integer"), IntegerEquality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"==@@integer16"), Integer16Equality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!=@@integer"), IntegerInequality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"==@@boolean"), BooleanEquality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!=@@boolean"), BooleanInequality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"==@@string"), StringEquality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!=@@string"), StringInequality));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L">@@integer"), IntegerGreaterThan));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"<@@integer"), IntegerLessThan));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"==@@real"), RealEquality));
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
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"==@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer16, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer16, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"==@@integer16"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!=@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Boolean, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Boolean, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"==@@boolean"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Boolean, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Boolean, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!=@@boolean"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_String, false);
		signature.AddParameter(L"i2", Metadata::EpochType_String, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"==@@string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_String, false);
		signature.AddParameter(L"i2", Metadata::EpochType_String, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!=@@string"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L">@@integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"<@@integer"), signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"==@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L">@@real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
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
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_COMPARISON));
	}
	{
		StringHandle handle = stringpool.Pool(L"!=");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_COMPARISON));
	}
	{
		StringHandle handle = stringpool.Pool(L">");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_COMPARISON));
	}
	{
		StringHandle handle = stringpool.Pool(L"<");
		AddToSetNoDupe(infixtable, stringpool.GetPooledString(handle));
		AddToMapNoDupe(precedences, std::make_pair(handle, PRECEDENCE_COMPARISON));
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
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"==@@integer16"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"==@@boolean"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"==@@string"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"==@@real"));
	}
	{
		StringHandle functionnamehandle = stringpool.Pool(L"!=");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!=@@integer"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!=@@boolean"));
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!=@@string"));
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

void ComparisonLibrary::RegisterJITTable(JIT::JITTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"<@@integer"), IntegerLessThanJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L">@@real"), RealGreaterThanJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"<@@real"), RealLessThanJIT));
}

