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

#include "Runtime/Runtime.h"

#include <vector>
#include <sstream>


extern Runtime::ExecutionContext* GlobalExecutionContext;		// TODO - use something more like the VM's GetThreadContext()


namespace
{

	StringHandle CmdLineIsValidHandle = 0;
	StringHandle CmdLineGetCountHandle = 0;
	StringHandle CmdLineGetHandle = 0;


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

}

//
// Bind the library to a function metadata table
//
void CommandLineLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.SetReturnType(Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(CmdLineIsValidHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(CmdLineGetCountHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"index", Metadata::EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(CmdLineGetHandle, signature));
	}
}


void CommandLineLibrary::PoolStrings(StringPoolManager& stringpool)
{
	CmdLineIsValidHandle = stringpool.Pool(L"cmdlineisvalid");
	CmdLineGetCountHandle = stringpool.Pool(L"cmdlinegetcount");
	CmdLineGetHandle = stringpool.Pool(L"cmdlineget");
}


void CommandLineLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.LibraryExports, std::make_pair(CmdLineIsValidHandle, "EpochLib_IsCmdLineSane"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(CmdLineGetCountHandle, "EpochLib_GetCmdLineCount"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(CmdLineGetHandle, "EpochLib_GetCmdLineToken"));
}


//
// Determine if the command line is sane
//
extern "C" bool EpochLib_IsCmdLineSane()
{
	return !ParseCommandLine().empty();
}

//
// Retrieve the number of parameters in the command line (including the process name)
//
extern "C" unsigned EpochLib_GetCmdLineCount()
{
	std::vector<std::wstring> cmdline(ParseCommandLine());
	return cmdline.size();
}

//
// Retrieve the token at the given index in the command line stream
//
extern "C" StringHandle EpochLib_GetCmdLineToken(unsigned index)
{
	std::vector<std::wstring> cmdline(ParseCommandLine());
	StringHandle handle;

	if(index >= cmdline.size())
		handle = GlobalExecutionContext->PoolString(L"");
	else
		handle = GlobalExecutionContext->PoolString(cmdline[index]);

	GlobalExecutionContext->TickStringGarbageCollector();
	return handle;
}

