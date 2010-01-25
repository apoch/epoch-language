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
	class ParserState;


	// Function declarations
	bool ParseFile(const std::string& filename, ParserState& state);

	bool ParseMemoryPass1(ParserState& state, const std::vector<Byte>& memblock, const std::string& sourcename);
	bool ParseMemoryPass2(ParserState& state, const std::vector<Byte>& memblock, const std::string& sourcename);
}

