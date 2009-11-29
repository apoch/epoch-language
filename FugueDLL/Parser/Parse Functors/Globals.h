//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - global variables and constants
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that we are entering a global definition block
//
struct EnterGlobalBlock : public ParseFunctorBase
{
	EnterGlobalBlock(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EnterGlobalBlock");

		State.EnterGlobalBlock();
	}
};

//
// Inform the parse analyzer that we are exiting a global definition block
//
struct ExitGlobalBlock : public ParseFunctorBase
{
	ExitGlobalBlock(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"ExitGlobalBlock");

		State.ExitGlobalBlock();
	}
};
