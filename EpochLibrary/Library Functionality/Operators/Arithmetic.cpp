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

#include "Metadata/Precedences.h"



namespace
{

	StringHandle AddHandle = 0;
	StringHandle SubtractHandle = 0;
	StringHandle MultiplyHandle = 0;
	StringHandle DivideHandle = 0;

	StringHandle AddToHandle = 0;
	StringHandle SubtractFromHandle = 0;

	StringHandle BangHandle = 0;
	StringHandle IncrementHandle = 0;
	StringHandle DecrementHandle = 0;

	StringHandle AddToRealsHandle = 0;
	StringHandle AddRealsHandle = 0;
	StringHandle SubtractFromRealsHandle = 0;
	StringHandle SubtractRealsHandle = 0;
	StringHandle MultiplyRealsHandle = 0;
	StringHandle DivideRealsHandle = 0;

	StringHandle AddToIntegersHandle = 0;
	StringHandle AddIntegersHandle = 0;
	StringHandle SubtractFromIntegersHandle = 0;
	StringHandle SubtractIntegersHandle = 0;
	StringHandle MultiplyIntegersHandle = 0;
	StringHandle DivideIntegersHandle = 0;

	StringHandle IncrementIntegersHandle = 0;
	StringHandle DecrementIntegersHandle = 0;

	StringHandle BitwiseNotHandle = 0;

	StringHandle IncrementRealsHandle = 0;
	StringHandle DecrementRealsHandle = 0;

	
	void IncrementIntegerJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateAdd(p1, llvm::ConstantInt::get(p1->getType(), 1));
		context.ValuesOnStack.push(result);
	}

	void DecrementIntegerJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateSub(p1, llvm::ConstantInt::get(p1->getType(), 1));
		context.ValuesOnStack.push(result);
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

	void SubtractIntegersJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateSub(p1, p2);
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

	void DivideIntegersJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* p1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* result = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateSDiv(p1, p2);
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
// Bind the library to a function metadata table
//
void ArithmeticLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(AddIntegersHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(SubtractIntegersHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(MultiplyIntegersHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(DivideIntegersHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(AddRealsHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(SubtractRealsHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(MultiplyRealsHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(DivideRealsHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(BitwiseNotHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(AddToIntegersHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(SubtractFromIntegersHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(AddToRealsHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"i1", Metadata::EpochType_Real, false);
		signature.AddParameter(L"i2", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(SubtractFromRealsHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(IncrementIntegersHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(DecrementIntegersHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(IncrementRealsHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"operand", Metadata::EpochType_Real, false);
		signature.SetReturnType(Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(DecrementRealsHandle, signature));
	}
}

//
// Bind the library to the infix operator table
//
void ArithmeticLibrary::RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences)
{
	AddToSetNoDupe(infixtable, L"+");
	AddToMapNoDupe(precedences, std::make_pair(AddHandle, PRECEDENCE_ADDSUBTRACT));

	AddToSetNoDupe(infixtable, L"-");
	AddToMapNoDupe(precedences, std::make_pair(SubtractHandle, PRECEDENCE_ADDSUBTRACT));

	AddToSetNoDupe(infixtable, L"*");
	AddToMapNoDupe(precedences, std::make_pair(MultiplyHandle, PRECEDENCE_MULTIPLYDIVIDE));

	AddToSetNoDupe(infixtable, L"/");
	AddToMapNoDupe(precedences, std::make_pair(DivideHandle, PRECEDENCE_MULTIPLYDIVIDE));
}

//
// Register the list of unary operators provided by the library module
//
void ArithmeticLibrary::RegisterUnaryOperators(StringSet& unaryprefixes, StringSet& preoperators, StringSet& postoperators)
{
	AddToSetNoDupe(unaryprefixes, L"!");

	AddToSetNoDupe(preoperators, L"++");
	AddToSetNoDupe(postoperators, L"++");

	AddToSetNoDupe(preoperators, L"--");
	AddToSetNoDupe(postoperators, L"--");
}

//
// Register the list of overloads used by functions in this library module
//
void ArithmeticLibrary::RegisterLibraryOverloads(OverloadMap& overloadmap)
{
	overloadmap[BangHandle].insert(BitwiseNotHandle);

	overloadmap[AddHandle].insert(AddIntegersHandle);
	overloadmap[AddHandle].insert(AddRealsHandle);

	overloadmap[SubtractHandle].insert(SubtractIntegersHandle);
	overloadmap[SubtractHandle].insert(SubtractRealsHandle);

	overloadmap[MultiplyHandle].insert(MultiplyIntegersHandle);
	overloadmap[MultiplyHandle].insert(MultiplyRealsHandle);

	overloadmap[DivideHandle].insert(DivideIntegersHandle);
	overloadmap[DivideHandle].insert(DivideRealsHandle);

	overloadmap[AddToHandle].insert(AddToIntegersHandle);
	overloadmap[AddToHandle].insert(AddToRealsHandle);

	overloadmap[SubtractFromHandle].insert(SubtractFromIntegersHandle);
	overloadmap[SubtractFromHandle].insert(SubtractFromRealsHandle);

	overloadmap[IncrementHandle].insert(IncrementIntegersHandle);
	overloadmap[IncrementHandle].insert(IncrementRealsHandle);

	overloadmap[DecrementHandle].insert(DecrementIntegersHandle);
	overloadmap[DecrementHandle].insert(DecrementRealsHandle);
}

//
// Register the list of operation-assignment operators provided by this library module
//
void ArithmeticLibrary::RegisterOpAssignOperators(StringSet& operators)
{
	AddToSetNoDupe(operators, L"+=");
	AddToSetNoDupe(operators, L"-=");
}

void ArithmeticLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(AddRealsHandle, &AddRealsJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(SubtractRealsHandle, &SubtractRealsJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(MultiplyRealsHandle, &MultiplyRealsJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(DivideRealsHandle, &DivideRealsJIT));

	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(AddToIntegersHandle, &AddIntegersJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(AddIntegersHandle, &AddIntegersJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(SubtractIntegersHandle, &SubtractIntegersJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(MultiplyIntegersHandle, &MultiplyIntegersJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(DivideIntegersHandle, &DivideIntegersJIT));

	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(IncrementIntegersHandle, &IncrementIntegerJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(DecrementIntegersHandle, &DecrementIntegerJIT));
}


void ArithmeticLibrary::PoolStrings(StringPoolManager& stringpool)
{
	AddHandle = stringpool.Pool(L"+");
	SubtractHandle = stringpool.Pool(L"-");
	MultiplyHandle = stringpool.Pool(L"*");
	DivideHandle = stringpool.Pool(L"/");

	AddToRealsHandle = stringpool.Pool(L"+=@@real");
	AddRealsHandle = stringpool.Pool(L"+@@real");
	SubtractFromRealsHandle = stringpool.Pool(L"-=@@real");
	SubtractRealsHandle = stringpool.Pool(L"-@@real");
	MultiplyRealsHandle = stringpool.Pool(L"*@@real");
	DivideRealsHandle = stringpool.Pool(L"/@@real");

	AddToIntegersHandle = stringpool.Pool(L"+=@@integer");
	AddIntegersHandle = stringpool.Pool(L"+@@integer");
	SubtractFromIntegersHandle = stringpool.Pool(L"-=@@integer");
	SubtractIntegersHandle = stringpool.Pool(L"-@@integer");
	MultiplyIntegersHandle = stringpool.Pool(L"*@@integer");
	DivideIntegersHandle = stringpool.Pool(L"/@@integer");

	IncrementIntegersHandle = stringpool.Pool(L"++@@integer");
	DecrementIntegersHandle = stringpool.Pool(L"--@@integer");

	BitwiseNotHandle = stringpool.Pool(L"!@@integer");

	IncrementRealsHandle = stringpool.Pool(L"++@@real");
	DecrementRealsHandle = stringpool.Pool(L"--@@real");

	AddToHandle = stringpool.Pool(L"+=");
	SubtractFromHandle = stringpool.Pool(L"-=");

	BangHandle = stringpool.Pool(L"!");
	IncrementHandle = stringpool.Pool(L"++");
	DecrementHandle = stringpool.Pool(L"--");
}

