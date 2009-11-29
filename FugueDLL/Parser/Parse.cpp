//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper functions for parsing code, either from disk or
// from an in-memory buffer.
//

#include "pch.h"

#include "Parser/Parse.h"
#include "Parser/Grammars.h"

#include "User Interface/Output.h"

#include "Utility/Files/Files.h"
#include "Utility/Strings.h"

#include "Configuration/RuntimeOptions.h"

#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/static_assert.hpp>


// Boost 1.38.0 has a problem compiling without
// this definition. I have no idea why.
namespace boost
{
	template <> struct STATIC_ASSERTION_FAILURE<false> { enum { value = 0 }; };
}


using namespace Parser;
using namespace boost::spirit::classic;


//
// Load a file into memory, then send it to the parser
//
bool Parser::ParseFile(EpochGrammarPreProcess& ppgrammar, EpochGrammar& grammar, const std::string& filename, std::vector<Byte>& memory)
{
	Files::Load(filename.c_str(), memory);
	ppgrammar.SetCodeBuffer(&memory[0]);
	return ParseMemory(ppgrammar, grammar, memory, filename);
}

//
// Parse a program stored in memory
//
bool Parser::ParseMemory(EpochGrammarPreProcess& ppgrammar, EpochGrammar& grammar, const std::vector<Byte>& memblock, const std::string& sourcename)
{
	boost::spirit::classic::position_iterator<const Byte*> start(&memblock[0], &memblock[0] + memblock.size() - 1, sourcename);
    boost::spirit::classic::position_iterator<const Byte*> end;

    SkipGrammar skip;

	parse_info<boost::spirit::classic::position_iterator<const Byte*> > result;

	UI::OutputStream out;
	out << L"Parsing: first pass..." << std::endl;

	try
	{
		result = parse(start, end, ppgrammar >> end_p, skip);
		if(!result.full || ppgrammar.GetState().ParseFailed)
			return false;
	}
	catch(std::exception& e)
	{
		ppgrammar.GetState().ReportFatalError(e.what());
		return false;
	}

	out << L"Parsing: second pass..." << std::endl;

	try
	{
		result = parse(start, end, grammar >> end_p, skip);
	}
	catch(std::exception& e)
	{
		grammar.GetState().ReportFatalError(e.what());
		return false;
	}

    return result.full && !grammar.GetState().ParseFailed;
}

