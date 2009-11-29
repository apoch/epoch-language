//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper functions for parsing code, either from disk or
// from an in-memory buffer.
//

#pragma once


namespace Parser
{
	// Forward declarations
	struct EpochGrammar;
	struct EpochGrammarPreProcess;
	class ParserState;

	// Function declarations
	bool ParseFile(EpochGrammarPreProcess& ppgrammar, EpochGrammar& grammar, const std::string& filename, std::vector<Byte>& memory);
	bool ParseMemory(EpochGrammarPreProcess& ppgrammar, EpochGrammar& grammar, const std::vector<Byte>& memblock, const std::string& sourcename);
}

