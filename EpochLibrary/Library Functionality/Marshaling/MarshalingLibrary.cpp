//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for data/code marshaling
//

#include "pch.h"

#include "Library Functionality/Marshaling/MarshalingLibrary.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Virtual Machine/Marshaling.h"

#include "Metadata/TypeInfo.h"


namespace
{

	StringHandle SizeOfHandle = 0;
	StringHandle MarshalStructureHandle = 0;


	void SizeOfJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		size_t size = context.Generator->OwnerVM.GetStructureDefinition(context.CurrentScope->GetVariableTypeByID(vartarget)).GetMarshaledSize();
		llvm::ConstantInt* csize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)), size);

		context.ValuesOnStack.push(csize);
	}

}


//
// Bind the library to a function metadata table
//
void MarshalingLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(SizeOfHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"pointer", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(MarshalStructureHandle, signature));
	}
}


void MarshalingLibrary::PoolStrings(StringPoolManager& stringpool)
{
	SizeOfHandle = stringpool.Pool(L"sizeof");
	MarshalStructureHandle = stringpool.Pool(L"marshalstructure");
}


void MarshalingLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(SizeOfHandle, &SizeOfJIT));
}

