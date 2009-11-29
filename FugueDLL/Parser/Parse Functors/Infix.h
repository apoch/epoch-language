//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - infix operators
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"
#include "Parser/Tracing.h"


//
// Inform the parse analyzer that an infix operator should be invoked
//
struct RegisterInfixOperator : public ParseFunctorBase
{
	RegisterInfixOperator(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterInfixOperator", std::wstring(begin, end));

		std::wstring op(begin, end);
		State.PushInfixOperator(op);
	}
};

//
// Inform the parse analyzer that an infix operand should be added to the current infix expression
//
struct RegisterInfixOperand : public ParseFunctorBase
{
	RegisterInfixOperand(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterInfixOperand", std::wstring(begin, end));

		State.RegisterInfixOperand();
	}
};

//
// Inform the parse analyzer that the current infix operand is an l-value,
// i.e. that we can assign an r-value into it.
//
struct RegisterInfixOperandAsLValue : public ParseFunctorBase
{
	RegisterInfixOperandAsLValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterInfixOperandAsLValue", str);

		State.RegisterInfixOperandAsLValue(str);
	}
};

//
// Inform the parse analyzer than an infix expression has ended and should be cleaned up
//
struct TerminateInfixExpression : public ParseFunctorBase
{
	TerminateInfixExpression(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"TerminateInfixExpression", std::wstring(begin, end));

		State.TerminateInfixExpression();
	}
};

//
// Reset the parse analyzer's tracking of infix expression data
// Generally this is used when a new expression is started, e.g. in a parameter list
//
struct ResetInfixTracking : public ParseFunctorBase
{
	ResetInfixTracking(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"ResetInfixTracking");

		State.ResetInfixTracking();
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"ResetInfixTracking");

		State.ResetInfixTracking();
	}
};

//
// Inform the parse analyzer that a parenthetical expression is being closed
//
struct TerminateParenthetical : public ParseFunctorBase
{
	TerminateParenthetical(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"TerminateParenthetical", std::wstring(begin, end));

		State.TerminateParenthetical();
	}
};

//
// Inject a prefix "not" operation
//
struct RegisterNotOperation : public ParseFunctorBase
{
	RegisterNotOperation(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterNotOperation", std::wstring(begin, end));

		State.RegisterNotOperation();
	}
};

//
// Inject a prefix "negate" operation
//
struct RegisterNegateOperation : public ParseFunctorBase
{
	RegisterNegateOperation(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"RegisterNegateOperation");

		State.RegisterNegateOperation();
	}
};

//
// Undo a "negate" operation that should not be present
//
// This is used to recover from partially-satisfied productions in the grammar.
//
struct UndoNegateOperation : public ParseFunctorBase
{
	UndoNegateOperation(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"UndoNegateOperation");

		State.UndoNegateOperation();
	}
};

//
// Inform the parse analyzer than the user wishes to define a custom infix operator
//
struct RegisterUserDefinedInfix : public ParseFunctorBase
{
	RegisterUserDefinedInfix(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterUserDefinedInfix", str);

		State.RegisterUserDefinedInfix(str);
	}
};

//
// Inform the parse analyzer of an operate-and-then-assign operation (e.g. +=)
//
struct RegisterOpAssignment : public ParseFunctorBase
{
	RegisterOpAssignment(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterOpAssignment", str);

		State.RegisterOpAssignment();
	}
};

//
// Inform the parse analyzer of an assignment operator
//
struct RegisterOpAssignmentOperator : public ParseFunctorBase
{
	RegisterOpAssignmentOperator(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterOpAssignmentOperator", str);

		State.RegisterOpAssignmentOperator(str);
	}
};




struct PreincrementVariable : public ParseFunctorBase
{
	PreincrementVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"PreincrementVariable", str);

		State.PreincrementVariable();
	}
};

struct PredecrementVariable : public ParseFunctorBase
{
	PredecrementVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"PredecrementVariable", str);

		State.PredecrementVariable();
	}
};

struct PostincrementVariable : public ParseFunctorBase
{
	PostincrementVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"PostincrementVariable", str);

		State.PostincrementVariable();
	}
};

struct PostdecrementVariable : public ParseFunctorBase
{
	PostdecrementVariable(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"PostdecrementVariable", str);

		State.PostdecrementVariable();
	}
};