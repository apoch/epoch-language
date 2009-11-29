//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Base class for parser semantic action functors
//

#pragma once


//
// Base class for all functors; provides access to the parser's
// state wrapper object.
//
struct ParseFunctorBase
{
// Construction
public:
	ParseFunctorBase(Parser::ParserState& state)
		: State(state)
	{ }

// Internal binding to state wrapper
protected:
	Parser::ParserState& State;
};

