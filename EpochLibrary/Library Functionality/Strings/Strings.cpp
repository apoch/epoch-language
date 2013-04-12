//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for string manipulation and processing
//

#include "pch.h"

#include "Library Functionality/Strings/Strings.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Runtime/Runtime.h"


extern Runtime::ExecutionContext* GlobalExecutionContext;


namespace
{

	StringHandle UnescapeHandle = 0;

	StringHandle SubstringHandle = 0;
	StringHandle SubstringWithLengthHandle = 0;
	StringHandle SubstringNoLengthHandle = 0;

}


//
// Bind the library to a function metadata table
//
void StringFunctionLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(UnescapeHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String, false);
		signature.AddParameter(L"start", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"length", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(SubstringWithLengthHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String, false);
		signature.AddParameter(L"start", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(SubstringNoLengthHandle, signature));
	}
}

void StringFunctionLibrary::RegisterLibraryOverloads(OverloadMap& overloadmap)
{
	overloadmap[SubstringHandle].insert(SubstringWithLengthHandle);
	overloadmap[SubstringHandle].insert(SubstringNoLengthHandle);
}


void StringFunctionLibrary::PoolStrings(StringPoolManager& stringpool)
{
	UnescapeHandle = stringpool.Pool(L"unescape");

	SubstringHandle = stringpool.Pool(L"substring");
	SubstringWithLengthHandle = stringpool.Pool(L"substring@@withlength");
	SubstringNoLengthHandle = stringpool.Pool(L"substring@@nolength");
}


void StringFunctionLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.LibraryExports, std::make_pair(SubstringWithLengthHandle, "EpochLib_SubstrLen"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(SubstringNoLengthHandle, "EpochLib_SubstrNoLen"));
}




extern "C" StringHandle EpochLib_SubstrLen(unsigned length, unsigned start, StringHandle strhandle)
{
	std::wstring slice = GlobalExecutionContext->OwnerVM.GetPooledString(strhandle).substr(start, length);
	return GlobalExecutionContext->OwnerVM.PoolString(slice);
}

extern "C" StringHandle EpochLib_SubstrNoLen(unsigned start, StringHandle strhandle)
{
	std::wstring slice = GlobalExecutionContext->OwnerVM.GetPooledString(strhandle).substr(start);
	return GlobalExecutionContext->OwnerVM.PoolString(slice);
}

