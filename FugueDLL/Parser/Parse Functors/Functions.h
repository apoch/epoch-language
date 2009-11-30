//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - functions
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that a function body is upcoming.
// Once the function's code block is closed, the function will
// be added to the current scope.
//
struct RegisterFunction : public ParseFunctorBase
{
	RegisterFunction(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunction", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring functionname(begin, end);
		State.RegisterUpcomingFunction(StripWhitespace(functionname));
	}
};

//
// Inform the parse analyzer that a function body is upcoming.
// This variant is used during the preparse phase.
//
struct RegisterFunctionPP : public ParseFunctorBase
{
	RegisterFunctionPP(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterFunctionPP", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring functionname(begin, end);
		State.RegisterUpcomingFunctionPP(StripWhitespace(functionname));
	}
};



//
// Inform the parse analyzer that an integer parameter has been defined for the current function.
//
struct RegisterIntegerParam : public ParseFunctorBase
{
	RegisterIntegerParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterIntegerParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingIntegerParam();
	}
};


//
// Inform the parse analyzer that a 16-bit integer parameter has been defined for the current function.
//
struct RegisterInt16Param : public ParseFunctorBase
{
	RegisterInt16Param(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterInt16Param", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingInt16Param();
	}
};

//
// Inform the parse analyzer that a real parameter has been defined for the current function.
//
struct RegisterRealParam : public ParseFunctorBase
{
	RegisterRealParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterRealParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingRealParam();
	}
};

//
// Inform the parse analyzer that a parameter of a user-defined type has been defined for the current function.
//
struct RegisterUnknownParam : public ParseFunctorBase
{
	RegisterUnknownParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUnknownParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterUpcomingUnknownParam(StripWhitespace(identifier));
	}
};

//
// Inform the parse analyzer that a string parameter has been defined for the current function.
//
struct RegisterStringParam : public ParseFunctorBase
{
	RegisterStringParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStringParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingStringParam();
	}
};

//
// Inform the parse analyzer that a boolean parameter has been defined for the current function.
//
struct RegisterBooleanParam : public ParseFunctorBase
{
	RegisterBooleanParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterBooleanParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingBooleanParam();
	}
};

//
// Inform the parse analyzer of the current parameter's name
//
struct RegisterParamName : public ParseFunctorBase
{
	RegisterParamName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterParamName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin, end);
		State.RegisterParam(StripWhitespace(value));
	}
};



//
// Inform the parse analyzer that a function's return value list is beginning.
//
struct RegisterBeginningOfFunctionReturns : public ParseFunctorBase
{
	RegisterBeginningOfFunctionReturns(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterBeginningOfFunctionReturns");

		State.RegisterBeginningOfFunctionReturns();
	}
};

//
// Inform the parse analyzer that the current function returns include an integer.
//
struct RegisterIntegerReturn : public ParseFunctorBase
{
	RegisterIntegerReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterIntegerReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring retname(begin, end);
		State.RegisterIntegerReturn(StripWhitespace(retname));
	}
};


//
// Inform the parse analyzer that the current function returns include a 16-bit integer.
//
struct RegisterInt16Return : public ParseFunctorBase
{
	RegisterInt16Return(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterInt16Return", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring retname(begin, end);
		State.RegisterInt16Return(StripWhitespace(retname));
	}
};

//
// Inform the parse analyzer that the current function returns include a real.
//
struct RegisterRealReturn : public ParseFunctorBase
{
	RegisterRealReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterRealReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring retname(begin, end);
		State.RegisterRealReturn(StripWhitespace(retname));
	}
};

//
// Inform the parse analyzer that the current function returns include a string.
//
struct RegisterStringReturn : public ParseFunctorBase
{
	RegisterStringReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStringReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring retname(begin, end);
		State.RegisterStringReturn(StripWhitespace(retname));
	}
};

//
// Inform the parse analyzer that the current function returns include a boolean value.
//
struct RegisterBooleanReturn : public ParseFunctorBase
{
	RegisterBooleanReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterBooleanReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring retname(begin, end);
		State.RegisterBooleanReturn(StripWhitespace(retname));
	}
};

//
// Inform the parse analyzer that the current function returns include a structure.
//
struct RegisterUnknownReturn : public ParseFunctorBase
{
	RegisterUnknownReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUnknownReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring rettypename(begin, end);
		State.RegisterUnknownReturn(StripWhitespace(rettypename));
	}
};

//
// Inform the parse analyzer of the name of the returned structure-typed value
//
struct RegisterUnknownReturnName : public ParseFunctorBase
{
	RegisterUnknownReturnName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUnknownReturnName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring retname(begin, end);
		State.RegisterUnknownReturnName(StripWhitespace(retname));
	}
};

//
// Inform the parse analyzer that we have finished the constructor of a structure-typed return value
// This function is used in the preparse phase to manage the parser state internally.
//
struct ExitUnknownReturnConstructor : public ParseFunctorBase
{
	ExitUnknownReturnConstructor(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"ExitUnknownReturnConstructor");

		State.SetParsePosition(begin);
		State.ExitUnknownReturnConstructor();
	}
};

//
// Inform the parse analyzer that we have finished the constructor of a structure-typed return value
// This function is used in the parse phase to set up initialization instructions for the members of
// the structure return variable being constructed.
//
struct FinishReturnConstructor : public ParseFunctorBase
{
	FinishReturnConstructor(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"FinishReturnConstructor");

		State.SetParsePosition(begin);
		State.FinishReturnConstructor();
	}
};

//
// Inform the parse analyzer that the current function returns nothing.
//
struct RegisterNullReturn : public ParseFunctorBase
{
	RegisterNullReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterNullReturn");

		State.RegisterNullReturn();
	}
};

//
// Inform the parse analyzer of the default value for a function return variable.
//
struct RegisterIntegerReturnValue : public ParseFunctorBase 
{
	RegisterIntegerReturnValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterIntegerReturnValue", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		Integer32 intvalue;
		std::wstringstream convert;
		convert << StripWhitespace(temp);
		if(!(convert >> intvalue))
			throw SyntaxException("Overflow in integer literal");
		State.RegisterReturnValue(intvalue);
	}
};

//
// Inform the parse analyzer of the default value for a function return variable.
//
struct RegisterInt16ReturnValue : public ParseFunctorBase 
{
	RegisterInt16ReturnValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterInt16ReturnValue", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		Integer16 intvalue;
		std::wstringstream convert;
		convert << StripWhitespace(temp);
		if(!(convert >> intvalue))
			throw SyntaxException("Overflow in integer literal");
		State.RegisterReturnValue(intvalue);
	}
};

//
// Inform the parse analyzer of the default value for a function return variable.
//
struct RegisterRealReturnValue : public ParseFunctorBase
{
	RegisterRealReturnValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterRealReturnValue", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		Real realvalue;
		std::wstringstream convert;
		convert << StripWhitespace(temp);
		if(!(convert >> realvalue))
			throw SyntaxException("Invalid real value");
		State.RegisterReturnValue(realvalue);
	}
};

//
// Inform the parse analyzer of the default value for a function return variable.
//
struct RegisterStringReturnValue : public ParseFunctorBase
{
	RegisterStringReturnValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStringReturnValue", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin + 1, end - 1);
		State.RegisterReturnValue(value);
	}
};

//
// Inform the parse analyzer of the default value for a function return variable.
//
struct RegisterBooleanReturnValue : public ParseFunctorBase
{
	RegisterBooleanReturnValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterBooleanReturnValue", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring temp(begin, end);
		temp = StripWhitespace(temp);
		if(temp == Keywords::True)
			State.RegisterReturnValue(true);
		else if(temp == Keywords::False)
			State.RegisterReturnValue(false);
		else
			throw_(begin, SYNTAXERROR_BOOLEANLITERAL);
	}
};
