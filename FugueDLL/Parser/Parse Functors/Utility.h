//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - utility operations used by the parse analyzer
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"
#include "Parser/Parser State Machine/ParserState.h"


//
// Store a string for later retrieval.
//
// Note that the parse analyzer provides several "slots" to place these
// strings into; this allows us to store strings for various purposes
// without stomping on another stored value.
//
struct SaveStringIdentifier : public ParseFunctorBase
{
	SaveStringIdentifier(Parser::ParserState& state, Parser::SavedStringIndex slotindex)
		: ParseFunctorBase(state), SlotIndex(slotindex)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"SaveStringIdentifier", std::wstring(begin, end));

		std::wstring identifier(begin, end);
		State.SaveStringIdentifier(StripWhitespace(identifier), SlotIndex);
	}

private:
	Parser::SavedStringIndex SlotIndex;
};

//
// Retrieve a stored string and push it onto the parse stack
//
struct PushSavedIdentifier : public ParseFunctorBase
{
	PushSavedIdentifier(Parser::ParserState& state, Parser::SavedStringIndex slotindex)
		: ParseFunctorBase(state), SlotIndex(slotindex)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushSavedIdentifier", std::wstring(begin, end));

		State.PushSavedIdentifier(SlotIndex);
	}

private:
	Parser::SavedStringIndex SlotIndex;
};


//
// Remove the trailing operations on the current code block for later use
//
struct CacheTailOperations : public ParseFunctorBase
{
	CacheTailOperations(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"CacheTailOperations", str);

		State.CacheTailOperations();
	}
};

//
// Place previously cached operations back onto the code block
//
struct PushCachedOperations : public ParseFunctorBase
{
	PushCachedOperations(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"PushCachedOperations");

		State.PushCachedOperations();
	}
};

