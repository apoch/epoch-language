//
// The Epoch Language Project
// Epoch Lexer for Scintilla
//
// Lexer implementation
//

#include "pch.h"

#include "Lexer.h"

#include <cassert>

#include "Scintilla/LexAccessor.h"
#include "Scintilla/Accessor.h"
#include "Scintilla/StyleContext.h"

#include <string>


static const int SCE_EPOCH_DEFAULT = 0;
static const int SCE_EPOCH_COMMENT = 1;
static const int SCE_EPOCH_TYPE = 2;
static const int SCE_EPOCH_STRINGLITERAL = 3;
static const int SCE_EPOCH_SYMBOL = 4;
static const int SCE_EPOCH_LITERAL = 5;
static const int SCE_EPOCH_UDT = 6;


namespace
{

	bool IsTypeKeyword(const char* rawtoken)
	{
		std::string token(rawtoken);

		return
		(
		   token == "integer"
		|| token == "integer16"
		|| token == "boolean"
		|| token == "string"
		|| token == "real"
		|| token == "buffer"
		);
	}

	bool IsUserDefinedTypeKeyword(const char* rawtoken)
	{
		std::string token(rawtoken);
		return false;
	}

	bool IsLiteralKeyword(const char* rawtoken)
	{
		std::string token(rawtoken);

		return
		(
		   token == "true"
		|| token == "false"
		);
	}

	bool IsTypeDeclarationKeyword(const char* rawtoken)
	{
		std::string token(rawtoken);

		return (token == "structure");
	}

}


EpochLexer::EpochLexer()
{
	// No implementation, just needed so that construction can be private
}

EpochLexer::~EpochLexer()
{
	// No implementation, just needed so that destruction can be private
}


void __stdcall EpochLexer::Release()
{
	delete this;
}

int __stdcall EpochLexer::Version() const
{
	return 1;
}

const char* __stdcall EpochLexer::PropertyNames()
{
	return "";
}

int __stdcall EpochLexer::PropertyType(const char* name)
{
	return 0;
}

const char* __stdcall EpochLexer::DescribeProperty(const char* name)
{
	return "";
}

int __stdcall EpochLexer::PropertySet(const char* key, const char* val)
{
	return -1;
}

const char* __stdcall EpochLexer::DescribeWordListSets()
{
	return "";
}

int __stdcall EpochLexer::WordListSet(int n, const char* wl)
{
	return -1;
}

void __stdcall EpochLexer::Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess)
{
	Accessor styler(pAccess, NULL);

	styler.StartAt(startPos);

	StyleContext sc(startPos, lengthDoc, initStyle, styler);

	for(; ; sc.Forward())
	{
		switch(sc.state)
		{
		case SCE_EPOCH_DEFAULT:
			if(sc.ch == '/' && sc.chNext == '/')
				sc.SetState(SCE_EPOCH_COMMENT);
			else if(sc.ch == '\"')
				sc.SetState(SCE_EPOCH_STRINGLITERAL);
			else if(sc.ch == ':')
				sc.SetState(SCE_EPOCH_SYMBOL);
			else if(sc.ch == '-' && sc.chNext == '>')
			{
				sc.SetState(SCE_EPOCH_SYMBOL);
				sc.Forward();
			}
			else if(isalpha(sc.ch))
				sc.SetState(SCE_EPOCH_TYPE);
			else if(isdigit(sc.ch) || (sc.ch == '-' && isdigit(sc.chNext)))
				sc.SetState(SCE_EPOCH_LITERAL);
			break;

		case SCE_EPOCH_COMMENT:
			if(sc.atLineEnd)
				sc.SetState(SCE_EPOCH_DEFAULT);
			break;

		case SCE_EPOCH_TYPE:
			if(!isalnum(sc.ch))
			{
				char token[512];
				sc.GetCurrent(token, sizeof(token));

				if(IsTypeKeyword(token))
					sc.ChangeState(SCE_EPOCH_TYPE);
				else if(IsLiteralKeyword(token))
					sc.ChangeState(SCE_EPOCH_LITERAL);
				else if(IsTypeDeclarationKeyword(token))
					sc.ChangeState(SCE_EPOCH_DEFAULT);
				else if(IsUserDefinedTypeKeyword(token))
					sc.ChangeState(SCE_EPOCH_UDT);
				else
					sc.ChangeState(SCE_EPOCH_DEFAULT);

				sc.SetState(SCE_EPOCH_DEFAULT);
			}
			break;

		case SCE_EPOCH_STRINGLITERAL:
			if(sc.ch == '\"')
			{
				sc.Forward();
				sc.SetState(SCE_EPOCH_DEFAULT);
			}
			break;

		case SCE_EPOCH_SYMBOL:
			sc.SetState(SCE_EPOCH_DEFAULT);
			break;

		case SCE_EPOCH_LITERAL:
			if(!isdigit(sc.ch) && sc.ch != '.')
				sc.SetState(SCE_EPOCH_DEFAULT);
			break;
		}

		if(!sc.More())
			break;
	}
	sc.SetState(SCE_EPOCH_DEFAULT);
	sc.Complete();
}

void __stdcall EpochLexer::Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess)
{
}

void* __stdcall EpochLexer::PrivateCall(int operation, void* pointer)
{
	return NULL;
}

