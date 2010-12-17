//
// The Epoch Language Project
// Epoch Lexer for Scintilla
//
// Factory for creation of lexer objects
//

#include "pch.h"

#include "Lexer.h"


ILexer* FactoryFunction()
{
	try
	{
		return new EpochLexer;
	}
	catch(...)
	{
		return NULL;
	}
}

