//
// The Epoch Language Project
// Epoch Lexer for Scintilla
//
// Prototypes for syntax highlighting implementation
//

#pragma once


// Dependencies
#include <string>


namespace Highlighter
{
	void UDTReset();
	void UDTAppend(const std::string& token);

	void UDFReset();
	void UDFAppend(const std::string& token);
}

