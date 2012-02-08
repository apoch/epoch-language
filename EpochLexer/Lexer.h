//
// The Epoch Language Project
// Epoch Lexer for Scintilla
//
// Lexer implementation
//

#pragma once


// Dependencies
#include "Scintilla/ILexer.h"


class EpochLexer : public ILexer
{
// Grant access to the factory
public:
	friend ILexer* FactoryFunction();

// Construction and destruction are limited to the factory and to the Release() interface
private:
	EpochLexer();
	~EpochLexer();

// ILexer interface functions
public:
	virtual void STDCALL Release();
	virtual int STDCALL Version() const;
	virtual const char* STDCALL PropertyNames();
	virtual int STDCALL PropertyType(const char* name);
	virtual const char* STDCALL DescribeProperty(const char* name);
	virtual int STDCALL PropertySet(const char* key, const char* val);
	virtual const char* STDCALL DescribeWordListSets();
	virtual int STDCALL WordListSet(int n, const char* wl);
	virtual void STDCALL Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess);
	virtual void STDCALL Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess);
	virtual void* STDCALL PrivateCall(int operation, void* pointer);
};

