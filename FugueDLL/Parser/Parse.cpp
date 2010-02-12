//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper functions for parsing code, either from disk or
// from an in-memory buffer.
//

#include "pch.h"

#include "Parser/Parse.h"
#include "Parser/Parser State Machine/ParserState.h"

#include "User Interface/Output.h"

#include "Utility/Files/Files.h"

#include "Configuration/RuntimeOptions.h"


using namespace Parser;


//
// Load a file into memory, then send it to the parser
//
bool Parser::ParseFile(const std::string& filename, ParserState& state, std::vector<Byte>& memory)
{
	Files::Load(filename.c_str(), memory);

	state.SetCodeBuffer(&memory[0]);
	
	if(!ParseMemoryPass1(state, memory, filename))
		return false;
	
	return ParseMemoryPass2(state, memory, filename);
}

