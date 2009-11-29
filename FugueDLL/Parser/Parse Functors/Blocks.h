//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - lexical blocks
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that a code block was just entered.
//
struct EnterBlock : public ParseFunctorBase
{
	EnterBlock(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EnterBlock");

		State.EnterBlock();
	}
};

//
// Inform the parse analyzer that a code block was just entered.
// This variant is used during the preparse phase.
//
struct EnterBlockPP : public ParseFunctorBase
{
	EnterBlockPP(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EnterBlockPP");

		State.EnterBlockPP();
	}
};

//
// Inform the parse analyzer that a code block was exited.
//
struct ExitBlock : public ParseFunctorBase
{
	ExitBlock(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"ExitBlock");

		State.ExitBlock();
	}
};

//
// Inform the parse analyzer that a code block was exited.
// This variant is used during the preparse phase.
//
struct ExitBlockPP : public ParseFunctorBase
{
	ExitBlockPP(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"ExitBlockPP");

		State.ExitBlockPP();
	}
};

