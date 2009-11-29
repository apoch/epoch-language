//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - type aliases
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that we want to set up a type alias
//
struct RegisterAliasName : public ParseFunctorBase
{
	RegisterAliasName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterAliasName", str);

		State.SetParsePosition(begin);
		State.SaveStringIdentifier(StripWhitespace(str), SavedStringSlot_Alias);
	}
};


//
// Inform the parse analyzer of what type we want to create an alias for
//
struct RegisterAliasType : public ParseFunctorBase
{
	RegisterAliasType(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterAliasType", str);

		State.SetParsePosition(begin);
		State.CreateTypeAlias(StripWhitespace(str));
	}
};


//
// Look up the actual type that backs an aliased type. This
// function places the resolved type name onto the parse
// stack, but does not generate any runtime operations.
//
struct ResolveAliasAndPushNoStack : public ParseFunctorBase
{
	ResolveAliasAndPushNoStack(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"ResolveAliasAndPushNoStack", str);

		State.SetParsePosition(begin);
		State.PushIdentifier(StripWhitespace(widen(State.ResolveAlias(str))));
		State.CountParameter();
	}
};

