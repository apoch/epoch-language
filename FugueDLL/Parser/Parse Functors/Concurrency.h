//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - concurrency
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Inform the parse analyzer that we are entering a task block
//
struct BeginTaskCode : public ParseFunctorBase
{
	BeginTaskCode(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginTaskCode");

		State.BeginTaskCode();
	}
};


//
// Inform the parse analyzer that we are entering a pooled thread block
//
struct BeginThreadCode : public ParseFunctorBase
{
	BeginThreadCode(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginThreadCode");

		State.BeginThreadCode();
	}
};


//
// Request the parse analyzer to save the name of a task for later retrieval
//
struct SaveTaskName : public ParseFunctorBase
{
	SaveTaskName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"SaveTaskName", str);

		State.SaveTaskName(str);
	}
};


//
// Note that the user has requested creation of a thread pool
//
struct RegisterThreadPool : public ParseFunctorBase
{
	RegisterThreadPool(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterThreadPool");
		State.RegisterThreadPool();
	}
};


//
// Inform the parse analyzer that we are entering a message dispatch block.
// This variant is called during the preparse phase.
//
struct RegisterUpcomingMessageDispatchPP : public ParseFunctorBase
{
	RegisterUpcomingMessageDispatchPP(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingMessageDispatchPP", std::wstring(begin, end));

		State.RegisterUpcomingMessageDispatch(true);
	}
};

//
// Inform the parse analyzer that we are entering a message dispatch block.
//
struct RegisterUpcomingMessageDispatch : public ParseFunctorBase
{
	RegisterUpcomingMessageDispatch(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterUpcomingMessageDispatch", std::wstring(begin, end));

		State.RegisterUpcomingMessageDispatch(false);
	}
};


//
// Inform the parse analyzer that the message being constructed has an integer parameter
//
struct RegisterIntegerMessageParam : public ParseFunctorBase
{
	RegisterIntegerMessageParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterIntegerMessageParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterIntegerMessageParam(StripWhitespace(identifier));
	}
};


//
// Inform the parse analyzer that the message being constructed has a 16-bit integer parameter
//
struct RegisterInt16MessageParam : public ParseFunctorBase
{
	RegisterInt16MessageParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterInt16MessageParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterInt16MessageParam(StripWhitespace(identifier));
	}
};


//
// Inform the parse analyzer that the message being constructed has a real parameter
//
struct RegisterRealMessageParam : public ParseFunctorBase
{
	RegisterRealMessageParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterRealMessageParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterRealMessageParam(StripWhitespace(identifier));
	}
};


//
// Inform the parse analyzer that the message being constructed has a boolean parameter
//
struct RegisterBooleanMessageParam : public ParseFunctorBase
{
	RegisterBooleanMessageParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterBooleanMessageParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterBooleanMessageParam(StripWhitespace(identifier));
	}
};

//
// Inform the parse analyzer that the message being constructed has a string parameter
//
struct RegisterStringMessageParam : public ParseFunctorBase
{
	RegisterStringMessageParam(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStringMessageParam", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStringMessageParam(StripWhitespace(identifier));
	}
};


//
// Inform the parse analyzer that we should begin building a scope
// description that represents a message's parameter set.
//
struct StartMessageParamScope : public ParseFunctorBase
{
	StartMessageParamScope(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StartMessageParamScope", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.StartMessageParamScope();
	}
};


//
// Request the parse analyzer to insert a function that retrieves the ID of the task's caller
//
// The caller is the task (including the main thread) which originally forked the task in question
//
struct PushCallerOperation : public ParseFunctorBase
{
	PushCallerOperation(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushCallerOperation", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.PushCallerOperation();
	}
};


//
// Request the parse analyzer to insert a function that retrieves the ID of a message's sender
//
// This allows a task to respond to any other task without needing to know the target task's ID.
//
struct PushSenderOperation : public ParseFunctorBase
{
	PushSenderOperation(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushSenderOperation", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.PushSenderOperation();
	}
};


//
// Inform the parse analyzer that we are entering a message response map block
//
struct BeginResponseMap : public ParseFunctorBase
{
	BeginResponseMap(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginResponseMap");

		State.BeginResponseMap();
	}
};

//
// Inform the parse analyzer that we are finished setting up a message response map block
//
struct EndResponseMap : public ParseFunctorBase
{
	EndResponseMap(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EndResponseMap");

		State.EndResponseMap();
	}
};

//
// Inform the parse analyzer of an entry in the message response map being constructed
//
struct RegisterResponseMapEntry : public ParseFunctorBase
{
	RegisterResponseMapEntry(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterResponseMapEntry", std::wstring(begin, end));

		State.RegisterResponseMapEntry();
	}
};

