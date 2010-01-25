//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Base class for Epoch parser grammars
//

#pragma once


namespace Parser
{

	// Forward declarations
	class ParserState;


	//
	// Shared base class for parser grammars; contains commonly
	// needed functionality for both the preprocess and the final
	// process grammars.
	//
	struct EpochGrammarBase
	{
	// Construction
	public:
		EpochGrammarBase(Parser::ParserState& state)
			: State(state)
		{ }

	// Access interface for retrieving the state tracker
	public:
		const Parser::ParserState& GetState() const
		{ return State; }

		Parser::ParserState& GetState()
		{ return State; }

	// Internal binding to the state tracker
	protected:
		Parser::ParserState& State;
	};

}

