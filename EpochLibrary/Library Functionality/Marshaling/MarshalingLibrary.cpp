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

