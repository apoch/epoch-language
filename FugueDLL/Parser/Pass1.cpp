#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"

#include "Parser/Parse.h"

#include "Parser/Grammars/Pass1.h"
#include "Parser/Grammars/Skip.h"

#include "User Interface/Output.h"


using namespace Parser;
using namespace boost::spirit::classic;


// Boost 1.38.0 has a problem compiling without
// this definition. I have no idea why.
namespace boost
{
	template <> struct STATIC_ASSERTION_FAILURE<false> { enum { value = 0 }; };
}


bool Parser::ParseMemoryPass1(ParserState& state, const std::vector<Byte>& memblock, const std::string& sourcename)
{
	position_iterator<const Byte*> start(&memblock[0], &memblock[0] + memblock.size() - 1, sourcename);
    position_iterator<const Byte*> end;

	EpochGrammarPreProcess ppgrammar(state);
    SkipGrammar skip;

	parse_info<position_iterator<const Byte*> > result;

	UI::OutputStream out;
	out << L"Parsing: first pass..." << std::endl;

	try
	{
		result = parse(start, end, ppgrammar >> end_p, skip);
		if(!result.full || state.ParseFailed)
			return false;
	}
	catch(std::exception& e)
	{
		state.ReportFatalError(e.what());
		return false;
	}

	return true;
}

