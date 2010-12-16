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

#include "Metadata/ActiveScope.h"


namespace
{
	void SizeOf(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle identifier = context.State.Stack.PopValue<StringHandle>();
		VM::EpochTypeID vartype = context.Variables->GetOriginalDescription().GetVariableTypeByID(identifier);

		if(vartype < VM::EpochType_CustomBase)
			context.State.Stack.PushValue(VM::GetStorageSize(vartype));
		else if(vartype > VM::EpochType_CustomBase)
			context.State.Stack.PushValue(context.OwnerVM.GetStructureDefinition(vartype).GetSize());
		else
			context.State.Stack.PushValue(0);
	}
}


//
// Bind the library to an execution dispatch table
//
void MarshalingLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"sizeof"), SizeOf));
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
}
