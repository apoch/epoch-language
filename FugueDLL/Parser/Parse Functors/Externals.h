//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - external libraries
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer of the name of the DLL where the external function resides.
//
struct RegisterExternalFunctionDLL : public ParseFunctorBase
{
	RegisterExternalFunctionDLL(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalFunctionDLL", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin + 1, end - 1);
		State.RegisterExternalDLL(value);
	}
};

//
// Inform the parse analyzer of an external function's identifier.
//
struct RegisterExternalFunctionName : public ParseFunctorBase
{
	RegisterExternalFunctionName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalFunctionName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin, end);
		State.RegisterExternalFunctionName(StripWhitespace(value));
	}
};

//
// Inform the parse analyzer that an external function accepts an integer parameter.
//
struct RegisterExternalIntegerParam : public ParseFunctorBase
{
	RegisterExternalIntegerParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalIntegerParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingIntegerParam();
	}
};

//
// Inform the parse analyzer that an external function accepts a 16-bit integer parameter.
//
struct RegisterExternalInt16Param : public ParseFunctorBase
{
	RegisterExternalInt16Param(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalInt16Param", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingInt16Param();
	}
};

//
// Inform the parse analyzer that an external function accepts a real parameter.
//
struct RegisterExternalRealParam : public ParseFunctorBase
{
	RegisterExternalRealParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalRealParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingRealParam();
	}
};

//
// Inform the parse analyzer that an external function accepts a string parameter.
//
struct RegisterExternalStringParam : public ParseFunctorBase
{
	RegisterExternalStringParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalStringParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingStringParam();
	}
};

//
// Inform the parse analyzer that an external function accepts a boolean parameter.
//
struct RegisterExternalBooleanParam : public ParseFunctorBase
{
	RegisterExternalBooleanParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalBooleanParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingBooleanParam();
	}
};

//
// Inform the parse analyzer that an external function accepts a byte buffer parameter.
//
struct RegisterExternalBufferParam : public ParseFunctorBase
{
	RegisterExternalBufferParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalBufferParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingBufferParam();
	}
};


//
// Inform the parse analyzer of an external function parameter's name.
//
struct RegisterExternalParamName : public ParseFunctorBase
{
	RegisterExternalParamName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalParamName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring value(begin, end);
		State.RegisterParam(StripWhitespace(value));
	}
};

//
// Inform the parse analyzer that an external function parameter should be passed by reference
//
struct RegisterParamIsReference : public ParseFunctorBase
{
	RegisterParamIsReference(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterParamIsReference", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterParamIsReference();
	}
};

//
// Inform the parse analyzer that an external function returns an integer value.
//
struct RegisterExternalIntegerReturn : public ParseFunctorBase
{
	RegisterExternalIntegerReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalIntegerReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterExternalIntegerReturn();
	}
};


//
// Inform the parse analyzer that an external function returns a 16-bit integer value.
//
struct RegisterExternalInt16Return : public ParseFunctorBase
{
	RegisterExternalInt16Return(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalInt16Return", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterExternalInt16Return();
	}
};

//
// Inform the parse analyzer that an external function returns a real value.
//
struct RegisterExternalRealReturn : public ParseFunctorBase
{
	RegisterExternalRealReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalRealReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterExternalRealReturn();
	}
};

//
// Inform the parse analyzer that an external function returns a string value.
//
struct RegisterExternalStringReturn : public ParseFunctorBase
{
	RegisterExternalStringReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalStringReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterExternalStringReturn();
	}
};

//
// Inform the parse analyzer that an external function returns a boolean value.
//
struct RegisterExternalBooleanReturn : public ParseFunctorBase
{
	RegisterExternalBooleanReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalBooleanReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterExternalBooleanReturn();
	}
};

//
// Inform the parse analyzer that an external function returns a byte buffer.
//
struct RegisterExternalBufferReturn : public ParseFunctorBase
{
	RegisterExternalBufferReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalBufferReturn", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterExternalBufferReturn();
	}
};

//
// Inform the parse analyzer that an external function returns nothing.
//
struct RegisterExternalNullReturn : public ParseFunctorBase
{
	RegisterExternalNullReturn(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterExternalNullReturn");

		State.RegisterExternalNullReturn();
	}
};

//
// Inform the parse analyzer that an external function accepts
// a pointer to the given type as a parameter
//
struct RegisterExternalParamAddressType : public ParseFunctorBase
{
	RegisterExternalParamAddressType(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalParamAddressType", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring nameoftype(begin, end);
		State.RegisterExternalParamAddressType(StripWhitespace(nameoftype));
	}
};

//
// Inform the parse analyzer that an external function accepts
// a pointer with the given name as a parameter
//
struct RegisterExternalParamAddressName : public ParseFunctorBase
{
	RegisterExternalParamAddressName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterExternalParamAddressName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring paramname(begin, end);
		State.RegisterExternalParamAddressName(StripWhitespace(paramname));
	}
};


//
// Import an Epoch-compatible library into the current program's parsing context
//
// This provides immediate access to the functions, variables, and types defined
// by the external library.
//
struct RegisterLibrary : public ParseFunctorBase
{
	RegisterLibrary(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterLibrary", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring filename(begin + 1, end - 1);
		State.RegisterLibrary(filename);
	}
};

