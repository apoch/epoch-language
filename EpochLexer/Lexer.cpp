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
#include "Scintilla/Scintilla.h"

#include <string>


static const int SCE_EPOCH_DEFAULT = 0;				// Default, unstyled tokens
static const int SCE_EPOCH_COMMENT = 1;				// Comments. Surprise, surprise.
static const int SCE_EPOCH_TYPE = 2;				// Built-in type keywords
static const int SCE_EPOCH_STRINGLITERAL = 3;		// String literals
static const int SCE_EPOCH_SYMBOL = 4;				// Special syntactically significant symbols
static const int SCE_EPOCH_LITERAL = 5;				// Non-string (i.e. numeric and boolean) literals
static const int SCE_EPOCH_HEXLITERAL = 6;			// Hexadecimal literals
static const int SCE_EPOCH_UDT = 7;					// User-defined types


namespace
{

	//
	// Determine if a given character is whitespace
	//
	bool IsWhitespaceChar(char c)
	{
		return (c == '\r' || c == '\n' || c == '\t' || c == ' ');
	}

	//
	// Determine if a given token corresponds to a built-in type keyword
	//
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

	//
	// Determine if a token corresponds to the name of a user-defined type
	//
	bool IsUserDefinedTypeKeyword(const char* rawtoken)
	{
		std::string token(rawtoken);
		// TODO - write a SemanticActionInterface which periodically parses the code and builds a list of UDTs for access here
		return false;
	}

	//
	// Determine if a token corresponds to a special literal keyword (e.g. boolean literals)
	//
	bool IsLiteralKeyword(const char* rawtoken)
	{
		std::string token(rawtoken);

		return
		(
		   token == "true"
		|| token == "false"
		);
	}

	//
	// Determine if a keyword is used for defining a user-defined type or alias
	//
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


//
// Destruct the lexer instance. This is done rather than having the external
// code delete the object directly in order to avoid nastiness with different
// compiler versions/implementations of memory management/etc. between DLLs.
//
void __stdcall EpochLexer::Release()
{
	delete this;
}

//
// Retrieve the version number of the lexer. We're lazy and just return 1.
//
int __stdcall EpochLexer::Version() const
{
	return 1;
}

//
// Return a list of all defined property names. Currently unused.
//
const char* __stdcall EpochLexer::PropertyNames()
{
	return "";
}

//
// Return the type of the given property. Currently unused.
//
int __stdcall EpochLexer::PropertyType(const char* name)
{
	return 0;
}

//
// Return the description of the given property. Currently unused.
//
const char* __stdcall EpochLexer::DescribeProperty(const char* name)
{
	return "";
}

//
// Set a property's value; currently unused and just pretends to succeeed.
//
int __stdcall EpochLexer::PropertySet(const char* key, const char* val)
{
	return -1;
}

//
// Return the descriptions of the word-lists used for syntax highlighting; currently unused.
//
const char* __stdcall EpochLexer::DescribeWordListSets()
{
	return "";
}

//
// Set a word list's contents. Currently unused and just pretends to succeed.
//
int __stdcall EpochLexer::WordListSet(int n, const char* wl)
{
	return -1;
}

//
// Now for the interesting bit! This function actually lexes the document and performs coloration.
//
// One word of advice: the names chosen by the StyleContext interface are rather poor. ChangeState()
// should be thought of as "change the state which will be applied to the current chunk of text" while
// SetState() should be thought of as "apply the selected state to the current chunk of text, then
// reset the state to the passed state value for further processing." With this in mind, it is much
// easier to understand how the lexer works.
//
// In a nutshell, we move through the document from the starting position through to the specified
// end length, one character at a time. We start in the provided state, which means we might already
// be in the middle of a token or something, and need to handle this situation correctly. To do the
// actual lexing work, we look at each character and see if it "belongs" in the current state. For
// instance, if we're parsing a numeric literal and encounter a letter, we know that the numeric
// literal state is no longer valid, and we need to switch to a new state. Each time states are changed,
// we use the ChangeState/SetState pair (elaborated on above) to accomplish the actual highlighting.
// In a few cases, such as when lexing a token that we haven't yet fully identified, we might need to
// retroactively change the state we were in based on the value of the token itself. This is done via
// the ChangeState function, and can be seen notably in the handler for SCE_EPOCH_TYPE.
//
void __stdcall EpochLexer::Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess)
{
	// Set up the handful of access interfaces needed for getting at the document itself
	Accessor styler(pAccess, NULL);
	styler.StartAt(startPos);
	StyleContext sc(startPos, lengthDoc, initStyle, styler);

	// Now lex! We basically just move forward relentlessly until we hit the end of the specified range.
	for(; ; sc.Forward())
	{
		switch(sc.state)
		{
		// From the default state, we need to be on the lookout for characters which signal a
		// required change into a new state.
		case SCE_EPOCH_DEFAULT:
			if(sc.ch == '/' && sc.chNext == '/')		// Check for comments
				sc.SetState(SCE_EPOCH_COMMENT);
			else if(sc.ch == '\"')						// Check for opening of a string literal
				sc.SetState(SCE_EPOCH_STRINGLITERAL);
			else if(sc.ch == ':')						// Check for special symbols
				sc.SetState(SCE_EPOCH_SYMBOL);
			else if(sc.ch == '-' && sc.chNext == '>')
			{
				sc.SetState(SCE_EPOCH_SYMBOL);
				sc.Forward();
			}
			else if(sc.ch == '0' && sc.chNext == 'x')	// Check for hexadecimal literals
			{
				sc.SetState(SCE_EPOCH_HEXLITERAL);
				sc.Forward();
			}
			else if(isalpha(sc.ch))						// Check for string tokens (see SCE_EPOCH_TYPE handler)
				sc.SetState(SCE_EPOCH_TYPE);
			else if(isdigit(sc.ch) || (sc.ch == '-' && isdigit(sc.chNext)))			// Lastly, check for numeric literals
				sc.SetState(SCE_EPOCH_LITERAL);
			break;

		// Comments terminate at the end of the current text line, no exceptions.
		case SCE_EPOCH_COMMENT:
			if(sc.atLineEnd)
				sc.SetState(SCE_EPOCH_DEFAULT);
			break;

		// We're lexing something which is a string token, but might fall into one of
		// several different actual categories. Once we detect the end of the token,
		// we examine the actual string value of that token and compare it with our
		// lists of keywords, user defined type names, function names, and so on. Then,
		// we use ChangeState() to retroactively switch to the correct highlighting
		// state, and lastly apply the state and return to default mode.
		case SCE_EPOCH_TYPE:
			if(!isalnum(sc.ch) && (sc.ch != '_'))
			{
				char token[512];		// arbitrary upper bound on the length of tokens that can be highlighted correctly
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

		// String literals end with a closing quote; no other processing is needed.
		// However, note that we skip forward to include the closing quote in the
		// text which is highlighted as being part of the string literal.
		case SCE_EPOCH_STRINGLITERAL:
			if(sc.ch == '\"')
			{
				sc.Forward();
				sc.SetState(SCE_EPOCH_DEFAULT);
			}
			break;

		// Symbols are usually short and handled manually elsewhere, so anything
		// that was a symbol will cause an immediate reset to the default state.
		case SCE_EPOCH_SYMBOL:
			sc.SetState(SCE_EPOCH_DEFAULT);
			break;

		// Literals are terminated with a byte that is not a digit or decimal.
		case SCE_EPOCH_LITERAL:
			if(!isdigit(sc.ch) && sc.ch != '.')
				sc.SetState(SCE_EPOCH_DEFAULT);
			break;

		// Hex literals are terminated with a byte that is not a hex digit
		case SCE_EPOCH_HEXLITERAL:
			if(!isdigit(sc.ch) && ((sc.ch < 'A' || sc.ch > 'F') && (sc.ch < 'a' || sc.ch > 'f')))
				sc.SetState(SCE_EPOCH_DEFAULT);
			break;
		}

		// Abort the loop if we've hit the end of our range
		if(!sc.More())
			break;
	}
	sc.SetState(SCE_EPOCH_DEFAULT);
	sc.Complete();
}

//
// Parse the document and set up folding (collapsable blocks)
//
void __stdcall EpochLexer::Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument* pAccess)
{
	Accessor styler(pAccess, NULL);
	styler.StartAt(startPos);

	int curline = styler.GetLine(startPos);
	unsigned endpos = startPos + lengthDoc;
	int visiblechars = 0;
	
	int levelprev = styler.LevelAt(curline) & SC_FOLDLEVELNUMBERMASK;
	int levelcur = levelprev;

	char chnext = styler[startPos];

	bool isbracealone = false;
	bool modifiedlevel = false;

	for(unsigned i = startPos; i < endpos; ++i)
	{
		char ch = chnext;
		chnext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);
		bool ateol = (ch == '\r' && chnext != '\n') || (ch == '\n');

		if(style != SCE_EPOCH_COMMENT && style != SCE_EPOCH_STRINGLITERAL)
		{
			if(ch == '{')
			{
				++levelcur;
				if(visiblechars > 0)
					isbracealone = false;
				else
					isbracealone = true;

				modifiedlevel = true;
			}
			else if(ch == '}')
			{
				--levelcur;
				modifiedlevel = true;
			}
		}

		if(ateol || (i == endpos - 1))
		{
			if(levelcur > levelprev)
			{
				if(isbracealone)
					styler.SetLevel(curline, levelcur);
				else
					styler.SetLevel(curline, levelprev);

				int effectiveline = curline - (curline && isbracealone ? 1 : 0);
				styler.SetLevel(effectiveline, styler.LevelAt(effectiveline) | SC_FOLDLEVELHEADERFLAG);
			}
			else if(levelcur < levelprev)
				styler.SetLevel(curline, levelprev);
			else
			{
				if(modifiedlevel && isbracealone)
				{
					if(curline)
					{
						styler.SetLevel(curline - 1, styler.LevelAt(curline - 1) | SC_FOLDLEVELHEADERFLAG);
						styler.SetLevel(curline, levelcur + 1);
					}
				}
				else
					styler.SetLevel(curline, levelcur);
			}

			++curline;
			levelprev = levelcur;
			visiblechars = 0;
			isbracealone = false;
		}

		if(!IsWhitespaceChar(ch))
			++visiblechars;
	}
}

//
// Unused mystery function.
//
void* __stdcall EpochLexer::PrivateCall(int operation, void* pointer)
{
	return NULL;
}
