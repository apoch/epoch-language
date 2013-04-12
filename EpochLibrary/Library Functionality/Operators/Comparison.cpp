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


extern Runtime::ExecutionContext* GlobalExecutionContext;


namespace
{

	StringHandle EqualityHandle = 0;
	StringHandle InequalityHandle = 0;
	StringHandle GreaterThanHandle = 0;
	StringHandle LessThanHandle = 0;

	StringHandle IntegerEqualityHandle = 0;
	StringHandle Integer16EqualityHandle = 0;
	StringHandle IntegerInequalityHandle = 0;
	StringHandle BooleanEqualityHandle = 0;
	StringHandle BooleanInequalityHandle = 0;
	StringHandle StringEqualityHandle = 0;
	StringHandle StringInequalityHandle = 0;
	StringHandle RealEqualityHandle = 0;

	StringHandle IntegerGreaterThanHandle = 0;
	StringHandle IntegerLessThanHandle = 0;
	StringHandle RealGreaterThanHandle = 0;
	StringHandle RealLessThanHandle = 0;


	void IntegerEqualityJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateICmp(llvm::CmpInst::ICMP_EQ, p1, p2);
		context.ValuesOnStack.push(flag);
	}

	void IntegerInequalityJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateICmp(llvm::CmpInst::ICMP_NE, p1, p2);
		context.ValuesOnStack.push(flag);
	}

	void IntegerGreaterThanJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateICmp(llvm::CmpInst::ICMP_SGT, p1, p2);
		context.ValuesOnStack.push(flag);
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

	void RealEqualityJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFCmp(llvm::CmpInst::FCMP_OEQ, p1, p2);
		context.ValuesOnStack.push(flag);
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

	void RealLessThanJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* flag = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateFCmp(llvm::CmpInst::FCMP_OLT, p1, p2);
		context.ValuesOnStack.push(flag);
	}

}


extern "C" bool EpochLib_StringEq(StringHandle b, StringHandle a)
{
	if(a == b)
		return true;

	return GlobalExecutionContext->OwnerVM.GetPooledString(a) == GlobalExecutionContext->OwnerVM.GetPooledString(b);
}

extern "C" bool EpochLib_StringIneq(StringHandle b, StringHandle a)
{
	if(a == b)
		return false;

	return GlobalExecutionContext->OwnerVM.GetPooledString(a) != GlobalExecutionContext->OwnerVM.GetPooledString(b);
}


//
// Bind the library to a function metadata table
//
void ComparisonLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(IntegerEqualityHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer16, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer16, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(Integer16EqualityHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(IntegerInequalityHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Boolean, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Boolean, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(BooleanEqualityHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Boolean, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Boolean, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(BooleanInequalityHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_String, false);
		signature.AddParameter(L"i2", Metadata::EpochType_String, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(StringEqualityHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_String, false);
		signature.AddParameter(L"i2", Metadata::EpochType_String, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(StringInequalityHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(IntegerGreaterThanHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(IntegerLessThanHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(RealEqualityHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(RealGreaterThanHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(RealLessThanHandle, signature));
	}
}

//
// Bind the library to the infix operator table
//
void ComparisonLibrary::RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences)
{
	{
		AddToSetNoDupe(infixtable, L"==");
		AddToMapNoDupe(precedences, std::make_pair(EqualityHandle, PRECEDENCE_COMPARISON));
	}
	{
		AddToSetNoDupe(infixtable, L"!=");
		AddToMapNoDupe(precedences, std::make_pair(InequalityHandle, PRECEDENCE_COMPARISON));
	}
	{
		AddToSetNoDupe(infixtable, L">");
		AddToMapNoDupe(precedences, std::make_pair(GreaterThanHandle, PRECEDENCE_COMPARISON));
	}
	{
		AddToSetNoDupe(infixtable, L"<");
		AddToMapNoDupe(precedences, std::make_pair(LessThanHandle, PRECEDENCE_COMPARISON));
	}
}


//
// Register the list of overloads used by functions in this library module
//
void ComparisonLibrary::RegisterLibraryOverloads(OverloadMap& overloadmap)
{
	overloadmap[EqualityHandle].insert(IntegerEqualityHandle);
	overloadmap[EqualityHandle].insert(Integer16EqualityHandle);
	overloadmap[EqualityHandle].insert(BooleanEqualityHandle);
	overloadmap[EqualityHandle].insert(StringEqualityHandle);
	overloadmap[EqualityHandle].insert(RealEqualityHandle);

	overloadmap[InequalityHandle].insert(IntegerInequalityHandle);
	overloadmap[InequalityHandle].insert(BooleanInequalityHandle);
	overloadmap[InequalityHandle].insert(StringInequalityHandle);

	overloadmap[GreaterThanHandle].insert(IntegerGreaterThanHandle);
	overloadmap[GreaterThanHandle].insert(RealGreaterThanHandle);

	overloadmap[LessThanHandle].insert(IntegerLessThanHandle);
	overloadmap[LessThanHandle].insert(RealLessThanHandle);
}

void ComparisonLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(IntegerEqualityHandle, &IntegerEqualityJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(Integer16EqualityHandle, &IntegerEqualityJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(RealEqualityHandle, &RealEqualityJIT));

	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(IntegerInequalityHandle, &IntegerInequalityJIT));

	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(IntegerGreaterThanHandle, &IntegerGreaterThanJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(IntegerLessThanHandle, &IntegerLessThanJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(RealGreaterThanHandle, &RealGreaterThanJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(RealLessThanHandle, &RealLessThanJIT));

	AddToMapNoDupe(table.LibraryExports, std::make_pair(StringEqualityHandle, "EpochLib_StringEq"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(StringInequalityHandle, "EpochLib_StringIneq"));
}


void ComparisonLibrary::PoolStrings(StringPoolManager& stringpool)
{
	EqualityHandle = stringpool.Pool(L"==");
	InequalityHandle = stringpool.Pool(L"!=");
	GreaterThanHandle = stringpool.Pool(L">");
	LessThanHandle = stringpool.Pool(L"<");

	IntegerEqualityHandle = stringpool.Pool(L"==@@integer");
	Integer16EqualityHandle = stringpool.Pool(L"==@@integer16");
	IntegerInequalityHandle = stringpool.Pool(L"!=@@integer");
	BooleanEqualityHandle = stringpool.Pool(L"==@@boolean");
	BooleanInequalityHandle = stringpool.Pool(L"!=@@boolean");
	StringEqualityHandle = stringpool.Pool(L"==@@string");
	StringInequalityHandle = stringpool.Pool(L"!=@@string");
	RealEqualityHandle = stringpool.Pool(L"==@@real");

	IntegerGreaterThanHandle = stringpool.Pool(L">@@integer");
	IntegerLessThanHandle = stringpool.Pool(L"<@@integer");
	RealGreaterThanHandle = stringpool.Pool(L">@@real");
	RealLessThanHandle = stringpool.Pool(L"<@@real");
}