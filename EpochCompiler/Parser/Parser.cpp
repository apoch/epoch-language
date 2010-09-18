//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for source code parsing functionality
//

#include "pch.h"

#include "Parser/Parser.h"
#include "Parser/Grammars.h"
#include "Parser/SemanticActionInterface.h"

#include "Utility/Strings.h"


using namespace boost::spirit::classic;


//
// Parse a given block of code, invoking the bound set of
// semantic actions during the parse process
//
bool Parser::Parse(const std::wstring& code)
{
	std::vector<char> memblock;
	memblock.reserve(code.length() + 20);		// paranoia padding

	std::string narrowedcode = narrow(code);
	std::copy(narrowedcode.begin(), narrowedcode.end(), std::back_inserter(memblock));

	// The parser prefers to have trailing whitespace, for whatever reason.
	memblock.push_back('\n');

	position_iterator<const char*> start(&memblock[0], &memblock[0] + memblock.size() - 1, "test");
    position_iterator<const char*> end;

	SemanticActions.SetPrepassMode(true);
	FundamentalGrammar grammar(SemanticActions, InfixIdentifiers);
    SkipGrammar skip;

	parse_info<position_iterator<const char*> > result;

	// TODO - exception handling here
	//try
	{
		// First pass: build up the list of entities defined in the code
		result = parse(start, end, grammar >> end_p, skip);
		if(!result.full)
			return false;

		// Second pass: traverse into each function and generate the corresponding bytecode
		SemanticActions.SetPrepassMode(false);
		result = parse(start, end, grammar >> end_p, skip);
		if(!result.full)
			return false;
	}
	//catch(std::exception& e)
	//{
	//	state.ReportFatalError(e.what());
	//	return false;
	//}

	return true;
}

