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


using namespace boost::spirit::qi;


//
// Parse a given block of code, invoking the bound set of
// semantic actions during the parse process
//
bool Parser::Parse(const std::wstring& code, const std::wstring& filename)
{
	SemanticActions.SetPrepassMode(true);
	FundamentalGrammar grammar(Identifiers);
    SkipGrammar skip;

	std::wstring::const_iterator iter = code.begin();
	std::wstring::const_iterator end = code.end();

	try
	{
		// First pass: build up the list of entities defined in the code
		AST::Program program;
		bool result = phrase_parse(iter, end, grammar, skip, program);
		if(!result || (iter != end))
			return false;
/*
		// Sanity check to make sure the parser is in a clean state
		SemanticActions.SanityCheck();

		// Don't do the second pass if prepass failed
		if(SemanticActions.DidFail())
			return false;

		// Second pass: traverse into each function and generate the corresponding bytecode
		do
		{
			AST::Program program;
			SemanticActions.SetPrepassMode(false);
			iter = code.begin();
			bool result = phrase_parse(iter, end, grammar, skip, program);
			if(!result || (iter != end))
				return false;
		} while(!SemanticActions.InferenceComplete());
		*/
	}
	catch(...)
	{
		throw FatalException("Exception during parse");
	}

	return true;
}

