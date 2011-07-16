//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for source code parsing functionality
//

#include "pch.h"

#include "Parser/Parser.h"
#include "Parser/Grammars.h"

#include "Compilation/SemanticActionInterface.h"

#include "Metadata/FunctionSignature.h"

#include "Utility/Strings.h"

#include "Utility/Profiling.h"


using namespace boost::spirit::qi;

#include <iostream>


//
// Parse a given block of code, invoking the bound set of
// semantic actions during the parse process
//
bool Parser::Parse(const std::wstring& code, const std::wstring& filename, AST::Program& program) const
{
	FundamentalGrammar grammar(Identifiers);
	SkipGrammar skip;

	std::wstring::const_iterator iter = code.begin();
	std::wstring::const_iterator end = code.end();

	try
	{
		// First pass: build up the list of entities defined in the code
		while(true)
		{
			Profiling::Timer timer;
			timer.Begin();

			program = AST::Program();
			std::wcout << L"Parsing... ";
			iter = code.begin();
			bool result = phrase_parse(iter, end, grammar, skip, program);
			if(!result || (iter != end))
			{
				std::wcout << L"FAILED!" << std::endl;
				return false;
			}

			timer.End();
			std::wcout << L"finished in " << timer.GetTimeMs() << L"ms" << std::endl;
		}
	}
	catch(...)
	{
		throw FatalException("Exception during parse");
	}

	return true;
}

