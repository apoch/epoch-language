//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - parameter passing
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Start counting the number of parameters passed to the current operation
//
struct StartCountingParams : public ParseFunctorBase
{
	StartCountingParams(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"StartCountingParams");

		State.StartCountingParams();
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StartCountingParams");

		State.StartCountingParams();
	}
};

//
// Push an integer literal value onto the parameter list.
//
struct PushIntegerLiteral : public ParseFunctorBase
{
	PushIntegerLiteral(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushIntegerLiteral", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		Integer32 intvalue;
		std::wstringstream convert;
		convert << StripWhitespace(temp);
		if(!(convert >> intvalue))
			throw SyntaxException("Overflow in integer literal");
		State.PushIntegerLiteral(intvalue);
	}
};

//
// Push a real literal value onto the parameter list.
//
struct PushRealLiteral : public ParseFunctorBase
{
	PushRealLiteral(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushRealLiteral", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		Real realvalue;
		std::wstringstream convert;
		convert << StripWhitespace(temp);
		if(!(convert >> realvalue))
			throw SyntaxException("Invalid real value");
		State.PushRealLiteral(realvalue);
	}
};

//
// Push a hex literal value onto the parameter list.
//
struct PushHexLiteral : public ParseFunctorBase
{
	PushHexLiteral(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushHexLiteral", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		unsigned value = 0;
		std::wstringstream convert;
		convert << std::hex << StripWhitespace(temp);
		convert >> value;
		State.PushIntegerLiteral(static_cast<Integer32>(value));
	}
};

//
// Push a string literal value onto the parameter list.
//
struct PushStringLiteral : public ParseFunctorBase
{
	PushStringLiteral(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushStringLiteral", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin + 1, end - 1);
		State.PushStringLiteral(value);
	}
};

//
// Push a string literal value onto the parse stack, but not onto the parameter list
//
struct PushStringLiteralNoStack : public ParseFunctorBase
{
	PushStringLiteralNoStack(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushStringLiteralNoStack", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin + 1, end - 1);
		State.PushStringLiteralNoStack(value);
	}
};

//
// Push a raw literal value onto the parameter list (mainly used when passing keywords, e.g. for cast operation)
//
struct PushRawStringNoStack : public ParseFunctorBase
{
	PushRawStringNoStack(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushRawStringNoStack", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin, end);
		State.PushStringLiteralNoStack(StripWhitespace(value));
	}
};

//
// Push a boolean literal value onto the parameter list.
//
struct PushBooleanLiteral : public ParseFunctorBase
{
	PushBooleanLiteral(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushBooleanLiteral", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		if(temp == Keywords::True)
			State.PushBooleanLiteral(true);
		else if(temp == Keywords::False)
			State.PushBooleanLiteral(false);
		else
			throw_(begin, SYNTAXERROR_BOOLEANLITERAL);
	}
};

//
// Push a variable identifier onto the parameter list.
//
struct PushIdentifierAsParameter : public ParseFunctorBase
{
	PushIdentifierAsParameter(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushIdentifierAsParameter", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.PushIdentifierAsParameter(StripWhitespace(identifier));
	}
};

//
// Push a variable identifier onto the parameter list.
// This variant does not push the variable's value onto the
// stack, but rather keeps a reference to the variable handy
// in order to do some operation on it later (e.g. this is
// used for assignment, where the identifier of the variable
// to write to is required, but its value shouldn't be on
// the stack prior to assignment).
//
struct PushIdentifierNoStack : public ParseFunctorBase
{
	PushIdentifierNoStack(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushIdentifierNoStack", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.PushIdentifier(StripWhitespace(identifier));
		State.CountParameter();
	}
};

struct PopParameterCount : public ParseFunctorBase
{
	PopParameterCount(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"PopParameterCount", str);

		State.PopParameterCount();
	}
};