//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - debugging
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Trip a breakpoint when the functor is called.
//
// This is useful for trapping control flow during a parse operation,
// and examining the parse analyzer's intermediate state.
//
struct DebugBreakFunctor : public ParseFunctorBase
{
	DebugBreakFunctor(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		__asm int 3;
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring parsedchunk(begin, end);
		__asm int 3;
	}
};


//
// Manually generate some parser trace output
//
// This functor is provided with a token from the parser, which is
// then displayed to the user in the parser trace output (if that
// output is enabled). This comes in handy for making sure that
// the grammar is providing the correct chunks of code to the
// parser, and, by extension, the parse analyzer.
//
struct DebugTraceFunctor : public ParseFunctorBase
{
	DebugTraceFunctor(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"{unknown token}");
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring parsedchunk(begin, end);
		Trace(parsedchunk.c_str());
	}
};


//
// Deliberately crash in the middle of parsing.
//
// This operation is useful for testing the error response
// of the parser code, particularly to detect possible memory
// leaks or other exception-safety issues.
//
struct CrashParser : public ParseFunctorBase
{
	CrashParser(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"FORCED CRASH");

		throw Exception("Parser deliberately crashed");
	}
};
