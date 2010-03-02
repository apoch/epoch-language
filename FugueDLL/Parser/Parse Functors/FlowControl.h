//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - program flow control
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that a flow control block is upcoming.
//
struct RegisterControl : public ParseFunctorBase
{
	RegisterControl(Parser::ParserState& state, bool preprocess)
		: ParseFunctorBase(state), Preprocess(preprocess)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterControl", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring controlname(begin, end);
		State.RegisterControl(StripWhitespace(controlname), Preprocess);
	}

protected:
	bool Preprocess;
};


//
// Inform the parse analyzer that a do/while loop was terminated.
//
struct PopDoWhileLoop : public ParseFunctorBase
{
	PopDoWhileLoop(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"PopDoWhileLoop");

		State.PopDoWhileLoop();
	}
};

//
// Inform the parse analyzer that a do/while loop was terminated.
// This variant is used during the preparse phase.
//
struct PopDoWhileLoopPP : public ParseFunctorBase
{
	PopDoWhileLoopPP(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"PopDoWhileLoopPP");

		State.PopDoWhileLoopPP();
	}
};

//
// Inform the parse analyzer that a while loop was terminated.
// This is used to inject the conditional check operation.
//
struct RegisterEndOfWhileLoopConditional : public ParseFunctorBase
{
	RegisterEndOfWhileLoopConditional(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterEndOfWhileLoopConditional");

		State.RegisterEndOfWhileLoopConditional();
	}
};


struct RegisterEndOfParallelFor : public ParseFunctorBase
{
	RegisterEndOfParallelFor(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterEndOfParallelFor");
		State.RegisterEndOfParallelFor();
	}
};


