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

#include "Metadata/ActiveScope.h"
#include "Metadata/TypeInfo.h"


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
		Metadata::EpochTypeID vartype = context.Variables->GetOriginalDescription().GetVariableTypeByID(identifier);

		if(Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_Primitive)
			context.State.Stack.PushValue(Metadata::GetMarshaledSize(vartype));
		else if(Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_TemplateInstance)
			context.State.Stack.PushValue(context.OwnerVM.GetStructureDefinition(vartype).GetMarshaledSize());
		else
			context.State.Result.ResultType = VM::ExecutionResult::EXEC_RESULT_HALT;
	}

	//
	// Marshal a structure from an external pointer into Epoch
	//
	void MarshalStructure(StringHandle, VM::ExecutionContext& context)
	{
		// WARNING - 64-bit compatibility issue here
		UINT_PTR pointer = static_cast<UINT_PTR>(context.State.Stack.PopValue<Integer32>());
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		Metadata::EpochTypeID vartype = context.Variables->GetOriginalDescription().GetVariableTypeByID(identifier);

		if(Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_TemplateInstance)
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
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"sizeof"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"pointer", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"marshalstructure"), signature));
	}
}
