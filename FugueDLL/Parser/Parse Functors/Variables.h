//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - variables
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that a string variable is being defined.
//
struct RegisterUpcomingStringVariable : public ParseFunctorBase
{
	RegisterUpcomingStringVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingStringVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_String);
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingStringVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_String);
	}
};

//
// Inform the parse analyzer that an integer variable is being defined.
//
struct RegisterUpcomingIntegerVariable : public ParseFunctorBase
{
	RegisterUpcomingIntegerVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingIntegerVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Integer);
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingIntegerVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Integer);
	}
};

//
// Inform the parse analyzer that a 16-bit integer variable is being defined.
//
struct RegisterUpcomingInt16Variable : public ParseFunctorBase
{
	RegisterUpcomingInt16Variable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingInt16Variable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Integer16);
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingInt16Variable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Integer16);
	}
};

//
// Inform the parse analyzer that a real variable is being defined.
//
struct RegisterUpcomingRealVariable : public ParseFunctorBase
{
	RegisterUpcomingRealVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingRealVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Real);
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingRealVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Real);
	}
};

//
// Inform the parse analyzer that a boolean variable is being defined.
//
struct RegisterUpcomingBooleanVariable : public ParseFunctorBase
{
	RegisterUpcomingBooleanVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingBooleanVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Boolean);
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingBooleanVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Boolean);
	}
};

//
// Inform the parse analyzer that the type of the following variable
// should be deduced via type inference from the initial value.
//
struct RegisterUpcomingInferredVariable : public ParseFunctorBase
{
	RegisterUpcomingInferredVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingInferredVariable");
		State.RegisterUpcomingInferredVariable();
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingInferredVariable");
		State.RegisterUpcomingInferredVariable();
	}
};

//
// Inform the parse analyzer that the following variable is of the buffer type.
//
struct RegisterUpcomingBufferVariable : public ParseFunctorBase
{
	RegisterUpcomingBufferVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingBufferVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Buffer);
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingBufferVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Buffer);
	}
};

//
// Inform the parse analyzer that the following variable is of the array container type.
//
struct RegisterUpcomingArrayVariable : public ParseFunctorBase
{
	RegisterUpcomingArrayVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterUpcomingArrayVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Array);
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingArrayVariable");
		State.RegisterUpcomingVariable(VM::EpochVariableType_Array);
	}
};

//
// Inform the parse analyzer of a defined variable's identifier.
//
struct RegisterVariableName : public ParseFunctorBase
{
	RegisterVariableName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterVariableName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterVariableName(StripWhitespace(identifier));
	}
};

//
// Inform the parse analyzer that the initial value for a variable has been passed
//
struct RegisterVariableValue : public ParseFunctorBase
{
	RegisterVariableValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterVariableValue", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterVariableValue();
	}
};

//
// Inform the parse analyzer that an array variable has been defined
//
struct RegisterArrayVariable : public ParseFunctorBase
{
	RegisterArrayVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterArrayVariable");

		State.RegisterArrayVariable();
	}
};

//
// Inform the parse analyzer that the next variable definition should be a constant
//
struct RegisterUpcomingConstant : public ParseFunctorBase
{
	RegisterUpcomingConstant(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingConstant", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.RegisterUpcomingConstant();
	}
};


//
// Store the type of an empty array for later retrieval and type validation
//
struct RegisterArrayType : public ParseFunctorBase
{
	RegisterArrayType(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterArrayType", str);

		State.SetParsePosition(begin);
		State.RegisterArrayType(str);
	}
};

