//
// The Epoch Language Project
// Epoch Lexer for Scintilla
//
// Exported functions for the lexer library
//

#include "pch.h"

#include "Factory.h"

#include <algorithm>


//
// Query interface: how many lexers are implemented in this DLL?
//
extern "C" int STDCALL GetLexerCount()
{
	return 1;
}

//
// Query interface: what's the name of the lexer with the given index?
//
extern "C" void STDCALL GetLexerName(unsigned index, char* name, int bufferlength)
{
	if(!name || bufferlength <= 0)
		return;

	*name = 0;

	if(index == 0)
	{
		const char* lexername = "epoch";
		memcpy(name, lexername, std::min(static_cast<unsigned>(bufferlength), strlen(lexername) + 1));
	}
}

//
// Factory interface: access the factory function that generates new lexer objects
//
extern "C" void* STDCALL GetLexerFactory(unsigned index)
{
	if(index == 0)
		return FactoryFunction;

	return 0;
}


