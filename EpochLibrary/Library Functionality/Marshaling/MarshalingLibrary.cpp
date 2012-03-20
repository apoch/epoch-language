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
#include "Virtual Machine/TypeInfo.h"
#include "Virtual Machine/Marshaling.h"

#include "Metadata/ActiveScope.h"


namespace
{
	//
	// Retrieve the size (in bytes) required to marshal a given variable into C APIs.
	//
	// Automatically accounts for structures, nested structures, field padding, etc. etc.
	//
	void SizeOf(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		VM::EpochTypeID vartype = context.Variables->GetOriginalDescription().GetVariableTypeByID(identifier);

		if(vartype < VM::EpochType_CustomBase)
			context.State.Stack.PushValue(VM::GetMarshaledSize(vartype));
		else if(vartype > VM::EpochType_CustomBase)
			context.State.Stack.PushValue(context.OwnerVM.GetStructureDefinition(vartype).GetMarshaledSize());
		else
			context.State.Stack.PushValue(0);
	}

	//
	// Marshal a structure from an external pointer into Epoch
	//
	void MarshalStructure(StringHandle, VM::ExecutionContext& context)
	{
		// WARNING - 64-bit compatibility issue here
		UINT_PTR pointer = static_cast<UINT_PTR>(context.State.Stack.PopValue<Integer32>());
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		VM::EpochTypeID vartype = context.Variables->GetOriginalDescription().GetVariableTypeByID(identifier);

		if(vartype > VM::EpochType_CustomBase)
		{
			const StructureDefinition& definition = context.OwnerVM.GetStructureDefinition(vartype);
			StructureHandle structhandle = context.Variables->Read<StructureHandle>(identifier);
			VM::MarshalBufferIntoStructureData(context, context.OwnerVM.GetStructure(structhandle), definition, reinterpret_cast<const Byte*>(pointer));
		}
	}
}


//
// Bind the library to an execution dispatch table
//
void MarshalingLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"sizeof"), SizeOf));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"marshalstructure"), MarshalStructure));
}

//
// Bind the library to a function metadata table
//
void MarshalingLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"sizeof"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"pointer", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"marshalstructure"), signature));
	}
}
