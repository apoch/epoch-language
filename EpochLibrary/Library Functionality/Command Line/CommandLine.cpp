//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for command line processing
//

#include "pch.h"

#include "Library Functionality/Command Line/CommandLine.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Strings.h"
#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Virtual Machine/VirtualMachine.h"

#include <vector>
#include <sstream>



namespace
{

	//
	// Helper for chunking the command line into string blocks
	//
	std::vector<std::wstring> ParseCommandLine()
	{
		std::vector<std::wstring> ret;
		std::wstring cmdline(::GetCommandLine());

		if(cmdline.empty())
			return ret;

		bool inquote = false;
		std::wstring quotedtoken;
		std::wstring token;

		std::wistringstream parser(cmdline);
		while(parser >> token)
		{
			token = StripWhitespace(token);
			if(token[0] == L'\"')
			{
				if(inquote && token.length() > 1)
				{
					ret.clear();
					return ret;
				}
				else
				{
					inquote = true;
					quotedtoken = L"";
					token = token.substr(1);
				}
			}
			
			if(*token.rbegin() == L'\"')
			{
				if(!inquote)
				{
					ret.clear();
					return ret;
				}

				inquote = false;
				if(token[0] == L'\"')
					quotedtoken = StripQuotes(token);
				else
					quotedtoken += L" " + token.substr(0, token.length() - 1);

				token = StripWhitespace(quotedtoken);
			}
			
			if(!inquote)
				ret.push_back(token);
			else
				quotedtoken += L" " + token;
		}

		return ret;
	}


	//
	// Determine if the command line is sane
	//
	void IsCmdLineSane(StringHandle functionname, VM::ExecutionContext& context)
	{
		context.State.Stack.PushValue(!ParseCommandLine().empty());
	}

	//
	// Retrieve the number of parameters in the command line (including the process name)
	//
	void GetCmdLineCount(StringHandle functionname, VM::ExecutionContext& context)
	{
		std::vector<std::wstring> cmdline(ParseCommandLine());
		context.State.Stack.PushValue(cmdline.size());
	}

	//
	// Retrieve the token at the given index in the command line stream
	//
	void GetCmdLineToken(StringHandle functionname, VM::ExecutionContext& context)
	{
		std::vector<std::wstring> cmdline(ParseCommandLine());
		size_t index = static_cast<size_t>(context.State.Stack.PopValue<Integer32>());
		StringHandle handle;

		if(index >= cmdline.size())
			handle = context.OwnerVM.PoolString(L"");
		else
			handle = context.OwnerVM.PoolString(cmdline[index]);

		context.State.Stack.PushValue(handle);
		context.TickStringGarbageCollector();
	}

}


//
// Bind the library to an execution dispatch table
//
void CommandLineLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cmdlineisvalid"), IsCmdLineSane));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cmdlinegetcount"), GetCmdLineCount));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"cmdlineget"), GetCmdLineToken));
}

//
// Bind the library to a function metadata table
//
void CommandLineLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cmdlineisvalid"), signature));
	}
	{
		FunctionSignature signature;
		signature.SetReturnType(VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cmdlinegetcount"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"index", VM::EpochType_Integer, false);
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"cmdlineget"), signature));
	}
}

