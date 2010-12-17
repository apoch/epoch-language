//
// The Epoch Language Project
// Epoch Lexer for Scintilla
//
// Exported functions for the lexer library
//

#include "pch.h"

#include "Factory.h"


//
// Query interface: how many lexers are implemented in this DLL?
//
extern "C" int __stdcall GetLexerCount()
{
	return 1;
}

//
// Query interface: what's the name of the lexer with the given index?
//
extern "C" void __stdcall GetLexerName(unsigned index, char* name, int bufferlength)
{
	if(!name)
		return;

	*name = 0;

	if(index == 0)
	{
		const char* lexername = "epoch";
		if(bufferlength > static_cast<int>(strlen(lexername)))
			strncpy(name, lexername, bufferlength - 1);
	}
}

//
// Factory interface: access the factory function that generates new lexer objects
//
extern "C" void* __stdcall GetLexerFactory(unsigned index)
{
	if(index == 0)
		return FactoryFunction;

	return 0;
}


