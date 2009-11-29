//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - tuples
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Register the name of a tuple type being defined, and prepare to fill its member list
//
struct RegisterTupleType : public ParseFunctorBase
{
	RegisterTupleType(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterTupleType", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterTupleType(StripWhitespace(identifier));
	}
};

//
// Register an integer-type member of a tuple
//
struct RegisterTupleIntegerMember : public ParseFunctorBase
{
	RegisterTupleIntegerMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterTupleIntegerMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterTupleMember(StripWhitespace(identifier), VM::EpochVariableType_Integer);
	}
};

//
// Register a 16-bit integer-type member of a tuple
//
struct RegisterTupleInt16Member : public ParseFunctorBase
{
	RegisterTupleInt16Member(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterTupleInt16Member", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterTupleMember(StripWhitespace(identifier), VM::EpochVariableType_Integer16);
	}
};

//
// Register a real-type member of a tuple
//
struct RegisterTupleRealMember : public ParseFunctorBase
{
	RegisterTupleRealMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterTupleRealMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterTupleMember(StripWhitespace(identifier), VM::EpochVariableType_Real);
	}
};

//
// Register a boolean-type member of a tuple
//
struct RegisterTupleBooleanMember : public ParseFunctorBase
{
	RegisterTupleBooleanMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterTupleBooleanMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterTupleMember(StripWhitespace(identifier), VM::EpochVariableType_Boolean);
	}
};

//
// Register a string-type member of a tuple
//
struct RegisterTupleStringMember : public ParseFunctorBase
{
	RegisterTupleStringMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterTupleStringMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterTupleMember(StripWhitespace(identifier), VM::EpochVariableType_String);
	}
};

//
// Notify the parse analyzer that the current tuple definition has
// been terminated. This is mainly used for last-minute validation,
// e.g. to ensure that there were some members added, etc.
//
struct FinishTupleType : public ParseFunctorBase
{
	FinishTupleType(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"FinishTupleType");

		State.FinishTupleType();
	}
};
