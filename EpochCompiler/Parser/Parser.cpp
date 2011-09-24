//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for source code parsing functionality
//

#include "pch.h"

#include "Parser/Parser.h"
#include "Parser/Grammars.h"

#include "Lexer/Lexer.h"
#include "Lexer/Lexer.inl"

#include "Compiler/Abstract Syntax Tree/Statement.h"

#include "Metadata/FunctionSignature.h"

#include "Utility/Strings.h"

#include "Utility/Profiling.h"


using namespace boost::spirit::qi;
using namespace boost::spirit::lex;

#include <iostream>


//
// Parse a given block of code, invoking the bound set of
// semantic actions during the parse process
//
bool Parser::Parse(const std::wstring& code, const std::wstring& filename, AST::Program& program) const
{
	std::auto_ptr<AST::Program> ptr(NULL);

	Lexer::EpochLexerT lexer;
	FundamentalGrammar grammar(lexer, Identifiers);

	std::wstring::const_iterator iter = code.begin();
	std::wstring::const_iterator end = code.end();

	try
	{
		// First pass: build up the list of entities defined in the code
		//while(true)
		{
			Profiling::Timer timer;
			timer.Begin();

			ptr.reset(new AST::Program);
			Memory::DisposeOneWayBlocks();

			std::wcout << L"Parsing... ";
			iter = code.begin();
			bool result = tokenize_and_parse(iter, end, lexer, grammar, *ptr);
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

	if(ptr.get())
		program = *ptr;

	return true;
}

