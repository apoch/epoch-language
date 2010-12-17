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
	virtual void __stdcall Release();
	virtual int __stdcall Version() const;
	virtual const char* __stdcall PropertyNames();
	virtual int __stdcall PropertyType(const char* name);
	virtual const char* __stdcall DescribeProperty(const char* name);
	virtual int __stdcall PropertySet(const char* key, const char* val);
	virtual const char* __stdcall DescribeWordListSets();
	virtual int __stdcall WordListSet(int n, const char* wl);
	virtual void __stdcall Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess);
	virtual void __stdcall Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess);
	virtual void* __stdcall PrivateCall(int operation, void* pointer);
};

