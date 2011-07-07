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

#include "Virtual Machine/VirtualMachine.h"



namespace
{

	//
	// Unescape a string (convert \x sequences into single bytes)
	//
	void UnescapeString(StringHandle functionname, VM::ExecutionContext& context)
	{
		const std::wstring& original = context.OwnerVM.GetPooledString(context.State.Stack.PopValue<StringHandle>());

		std::wstring result;
		result.reserve(original.length() + 1);

		for(size_t i = 0; i < original.length(); ++i)
		{
			if(original[i] == L'\\')
			{
				++i;
				switch(original[i])
				{
				case L'0':		result += L'\0';		break;
				case L'r':		result += L'\r';		break;
				case L'n':		result += L'\n';		break;
				case L't':		result += L'\t';		break;
				case L'\\':		result += L'\\';		break;
				default:		result += L'?';			break;
				}
			}
			else
				result += original[i];
		}

		StringHandle handle = context.OwnerVM.PoolString(result);
		context.State.Stack.PushValue(handle);
		context.TickStringGarbageCollector();
	}

}


//
// Bind the library to an execution dispatch table
//
void StringFunctionLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"unescape"), UnescapeString));
}

//
// Bind the library to a function metadata table
//
void StringFunctionLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", VM::EpochType_String, false);
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"unescape"), signature));
	}
}

