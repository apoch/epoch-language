//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - function signatures and first class functions
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer of the name of a function signature used as
// a higher-order function parameter. The function can be invoked via
// this name from within the function being defined.
//
struct RegisterFunctionSignatureName : public ParseFunctorBase
{
	RegisterFunctionSignatureName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterFunctionSignatureName(StripWhitespace(identifier));
	}
};

//
// Inform the parse analyzer of the parameters in a function signature
//
struct RegisterFunctionSignatureIntegerParam : public ParseFunctorBase
{
	RegisterFunctionSignatureIntegerParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureIntegerParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureParam(VM::EpochVariableType_Integer);
	}
};

struct RegisterFunctionSignatureInteger16Param : public ParseFunctorBase
{
	RegisterFunctionSignatureInteger16Param(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureInteger16Param", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureParam(VM::EpochVariableType_Integer16);
	}
};

struct RegisterFunctionSignatureStringParam : public ParseFunctorBase
{
	RegisterFunctionSignatureStringParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureStringParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureParam(VM::EpochVariableType_String);
	}
};

struct RegisterFunctionSignatureBooleanParam : public ParseFunctorBase
{
	RegisterFunctionSignatureBooleanParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureBooleanParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureParam(VM::EpochVariableType_Boolean);
	}
};

struct RegisterFunctionSignatureRealParam : public ParseFunctorBase
{
	RegisterFunctionSignatureRealParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureRealParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureParam(VM::EpochVariableType_Real);
	}
};

struct RegisterFunctionSignatureUnknownParam : public ParseFunctorBase
{
	RegisterFunctionSignatureUnknownParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureUnknownParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterFunctionSignatureParam(StripWhitespace(identifier));
	}
};

//
// Inform the parse analyzer that a function parameter is to be passed by reference
//
struct RegisterFunctionSignatureParamIsReference : public ParseFunctorBase
{
	RegisterFunctionSignatureParamIsReference(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureParamIsReference", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureParamIsReference();
	}
};

//
// Inform the parse analyzer of the return values in a function signature
//
struct RegisterFunctionSignatureIntegerReturn : public ParseFunctorBase
{
	RegisterFunctionSignatureIntegerReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureIntegerReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureReturn(VM::EpochVariableType_Integer);
	}
};

struct RegisterFunctionSignatureInteger16Return : public ParseFunctorBase
{
	RegisterFunctionSignatureInteger16Return(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureInteger16Return", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureReturn(VM::EpochVariableType_Integer16);
	}
};

struct RegisterFunctionSignatureStringReturn : public ParseFunctorBase
{
	RegisterFunctionSignatureStringReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureStringReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureReturn(VM::EpochVariableType_String);
	}
};

struct RegisterFunctionSignatureBooleanReturn : public ParseFunctorBase
{
	RegisterFunctionSignatureBooleanReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureBooleanReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureReturn(VM::EpochVariableType_Boolean);
	}
};

struct RegisterFunctionSignatureRealReturn : public ParseFunctorBase
{
	RegisterFunctionSignatureRealReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureRealReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterFunctionSignatureReturn(VM::EpochVariableType_Real);
	}
};

struct RegisterFunctionSignatureUnknownReturn : public ParseFunctorBase
{
	RegisterFunctionSignatureUnknownReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionSignatureUnknownReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterFunctionSignatureReturn(StripWhitespace(identifier));
	}
};

//
// Register the end of a function signature definition
//
struct RegisterFunctionSignatureEnd : public ParseFunctorBase
{
	RegisterFunctionSignatureEnd(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterFunctionSignatureEnd");

		State.RegisterFunctionSignatureEnd();
	}
};
