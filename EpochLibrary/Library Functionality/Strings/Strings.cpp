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

	StringHandle SubCharacterHandle = 0;

}


//
// Bind the library to a function metadata table
//
void StringFunctionLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(UnescapeHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String);
		signature.AddParameter(L"start", Metadata::EpochType_Integer);
		signature.AddParameter(L"length", Metadata::EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(SubstringWithLengthHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String);
		signature.AddParameter(L"start", Metadata::EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(SubstringNoLengthHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", Metadata::EpochType_String);
		signature.AddParameter(L"start", Metadata::EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(SubCharacterHandle, signature));
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

	SubCharacterHandle = stringpool.Pool(L"subchar");
}


void StringFunctionLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.LibraryExports, std::make_pair(SubstringWithLengthHandle, "EpochLib_SubstrLen"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(SubstringNoLengthHandle, "EpochLib_SubstrNoLen"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(UnescapeHandle, "EpochLib_Unescape"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(SubCharacterHandle, "EpochLib_SubstrChar"));
}




extern "C" StringHandle EpochLib_SubstrLen(unsigned length, unsigned start, StringHandle strhandle)
{
	std::wstring slice = GlobalExecutionContext->GetPooledString(strhandle).substr(start, length);
	return GlobalExecutionContext->PoolString(slice);
}

extern "C" StringHandle EpochLib_SubstrNoLen(unsigned start, StringHandle strhandle)
{
	std::wstring slice = GlobalExecutionContext->GetPooledString(strhandle).substr(start);
	return GlobalExecutionContext->PoolString(slice);
}

extern "C" int EpochLib_SubstrChar(unsigned start, StringHandle strhandle)
{
	return GlobalExecutionContext->GetPooledString(strhandle)[start];
}

extern "C" StringHandle EpochLib_Unescape(StringHandle strhandle)
{
	const std::wstring& original = GlobalExecutionContext->GetPooledString(strhandle);

	std::wstring result;
	result.reserve(original.length() + 1);

	for(size_t i = 0; i < original.length(); ++i)
	{
		if(original[i] == L'\\')
		{
			++i;
			switch(original[i])
			{
			case L'0':              result += L'\0';                break;
			case L'r':              result += L'\r';                break;
			case L'n':              result += L'\n';                break;
			case L't':              result += L'\t';                break;
			case L'\\':             result += L'\\';                break;
			case L'\'':				result += L'"';					break;		// TODO - stupid hack. Support escaped strings in the parser.
			default:                result += L'?';                 break;
			}
		}
		else
			result += original[i];
	}

	StringHandle handle = GlobalExecutionContext->PoolString(result);
	
	return handle;
}

