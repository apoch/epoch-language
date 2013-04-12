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
	
	StringHandle BangHandle = 0;
	StringHandle BooleanNotHandle = 0;

	//
	// Compute the logical negation of an operand
	//
	void BooleanNotJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* v = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* notv = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateNot(v);
		context.ValuesOnStack.push(notv);
	}

}


//
// Bind the library to a function metadata table
//
void BooleanLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"b", Metadata::EpochType_Boolean, false);
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
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(BooleanNotHandle, &BooleanNotJIT));
}


void BooleanLibrary::PoolStrings(StringPoolManager& stringpool)
{
	BangHandle = stringpool.Pool(L"!");
	BooleanNotHandle = stringpool.Pool(L"!@@boolean");
}