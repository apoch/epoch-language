//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for boolean operators
//

#include "pch.h"

#include "Library Functionality/Operators/Boolean.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Libraries/Library.h"

#include "Runtime/Runtime.h"

#include "Metadata/Precedences.h"


namespace
{
	
	StringHandle AndHandle = 0;
	StringHandle BangHandle = 0;
	StringHandle BooleanNotHandle = 0;

	//
	// Compute the logical negation of an operand
	//
	bool BooleanNotJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* v = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* notv = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateNot(v);
		context.ValuesOnStack.push(notv);

		return true;
	}

	//
	// Compute logical and of two operands
	//
	bool BooleanAndJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* v2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* v1 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* andvs = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateAnd(v1, v2);
		context.ValuesOnStack.push(andvs);

		return true;
	}

}


//
// Bind the library to a function metadata table
//
void BooleanLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"b1", Metadata::EpochType_Boolean);
		signature.AddParameter(L"b2", Metadata::EpochType_Boolean);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(AndHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"b", Metadata::EpochType_Boolean);
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(BooleanNotHandle, signature));
	}
}

//
// Register the list of overloads used by functions in this library module
//
void BooleanLibrary::RegisterLibraryOverloads(OverloadMap& overloadmap)
{
	overloadmap[BangHandle].insert(BooleanNotHandle);
}

void BooleanLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(AndHandle, &BooleanAndJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(BooleanNotHandle, &BooleanNotJIT));
}


//
// Bind the library to the infix operator table
//
void BooleanLibrary::RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences)
{
	AddToSetNoDupe(infixtable, L"&&");
	AddToMapNoDupe(precedences, std::make_pair(AndHandle, PRECEDENCE_BITWISE));		// TODO - is this sane?
}



void BooleanLibrary::PoolStrings(StringPoolManager& stringpool)
{
	AndHandle = stringpool.Pool(L"&&");
	BangHandle = stringpool.Pool(L"!");
	BooleanNotHandle = stringpool.Pool(L"!@@boolean");
}

