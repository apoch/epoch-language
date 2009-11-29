//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - operation management
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that an operation is about to
// be called; the parameters will follow in the input. This
// functor is used for standalone operation calls.
//
struct RegisterOperation : public ParseFunctorBase
{
	RegisterOperation(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterOperation", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring operationname(begin, end);
		operationname = StripWhitespace(operationname);

		if(operationname.substr(0, 2) == Operators::Increment || operationname.substr(operationname.length() - 2) == Operators::Increment)
		{
			State.MergeDeferredOperations();
			return;
		}
		if(operationname.substr(0, 2) == Operators::Decrement || operationname.substr(operationname.length() - 2) == Operators::Decrement)
		{
			State.MergeDeferredOperations();
			return;
		}

		operationname = operationname.substr(0, operationname.find(L'('));

		State.PushOperation(StripWhitespace(operationname));
	}
};

//
// Inform the parse analyzer that an operation is about to
// be called; the parameters will follow in the input. This
// functor is used when an operation's return value is passed
// as a parameter to another operation.
//
struct RegisterOperationAsParameter : public ParseFunctorBase
{
	RegisterOperationAsParameter(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterOperationAsParameter", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring operationname(begin, end);
		
		if(operationname.substr(0, 2) == Operators::Increment || operationname.substr(operationname.length() - 2) == Operators::Increment)
		{
			State.HandleInlineIncDec();
			return;
		}
		if(operationname.substr(0, 2) == Operators::Decrement || operationname.substr(operationname.length() - 2) == Operators::Decrement)
		{
			State.HandleInlineIncDec();
			return;
		}
		
		operationname = operationname.substr(0, operationname.find(L'('));

		State.PushOperationAsParameter(StripWhitespace(operationname));
	}
};
