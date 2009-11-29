//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser semantic action functors - structures
//

#pragma once

#include "Parser/Parse Functors/ParseFunctorBase.h"


//
// Register the name of a structure type being defined, and prepare to fill its member list
//
struct RegisterStructureType : public ParseFunctorBase
{
	RegisterStructureType(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureType", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureType(StripWhitespace(identifier));
	}
};

//
// Register an integer-type member of a structure
//
struct RegisterStructureIntegerMember : public ParseFunctorBase
{
	RegisterStructureIntegerMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureIntegerMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureMember(StripWhitespace(identifier), VM::EpochVariableType_Integer);
	}
};

//
// Register a 16-bit integer-type member of a structure
//
struct RegisterStructureInt16Member : public ParseFunctorBase
{
	RegisterStructureInt16Member(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureInt16Member", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureMember(StripWhitespace(identifier), VM::EpochVariableType_Integer16);
	}
};

//
// Register a real-type member of a structure
//
struct RegisterStructureRealMember : public ParseFunctorBase
{
	RegisterStructureRealMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureRealMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureMember(StripWhitespace(identifier), VM::EpochVariableType_Real);
	}
};

//
// Register a boolean-type member of a structure
//
struct RegisterStructureBooleanMember : public ParseFunctorBase
{
	RegisterStructureBooleanMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureBooleanMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureMember(StripWhitespace(identifier), VM::EpochVariableType_Boolean);
	}
};

//
// Register a string-type member of a structure
//
struct RegisterStructureStringMember : public ParseFunctorBase
{
	RegisterStructureStringMember(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureStringMember", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureMember(StripWhitespace(identifier), VM::EpochVariableType_String);
	}
};

//
// Register a nested structure within a parent structure
//
struct RegisterStructureUnknown : public ParseFunctorBase
{
	RegisterStructureUnknown(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureUnknown", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureMemberUnknown(StripWhitespace(identifier));
	}
};

//
// Register the type of an upcoming nested structure member
//
struct RegisterStructureUnknownTypeName : public ParseFunctorBase
{
	RegisterStructureUnknownTypeName(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterStructureUnknownTypeName", std::wstring(begin, end));

		State.SetParsePosition(begin);
		std::wstring identifier(begin, end);
		State.RegisterStructureUnknownTypeName(StripWhitespace(identifier));
	}
};

//
// Notify the parse analyzer that the current structure definition has
// been terminated. This is mainly used for last-minute validation,
// e.g. to ensure that there were some members added, etc.
//
struct FinishStructureType : public ParseFunctorBase
{
	FinishStructureType(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"FinishStructureType");

		State.FinishStructureType();
	}
};

//
// Note that we are parsing inside a call to the member() function.
// Tracking this is important so we can do type validation by peeking
// at the instructions around the member() call.
//
struct IncrementMemberLevel : public ParseFunctorBase
{
	IncrementMemberLevel(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"IncrementMemberLevel", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.IncrementMemberLevel();
	}
};

//
// Reset the tracking of member() calls after they have exited
//
struct ResetMemberLevel : public ParseFunctorBase
{
	ResetMemberLevel(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"ResetMemberLevel", std::wstring(begin, end));

		State.SetParsePosition(begin);
		State.ResetMemberLevel();
	}
};

//
// Inform the parse analyzer that a structure member is being accessed
//
struct RegisterMemberAccess : public ParseFunctorBase
{
	RegisterMemberAccess(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterMemberAccess", str);

		State.SetParsePosition(begin);
		State.RegisterMemberAccess(str);
	}
};

//
// Reset tracking of how deep we are in chained member accesses
//
struct ResetMemberAccess : public ParseFunctorBase
{
	ResetMemberAccess(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"ResetMemberAccess", std::wstring(begin, end));
		State.ResetMemberAccess();
	}
};

//
// Inform the parse analyzer that we are accessing a structure member and using it as an l-value
//
struct RegisterMemberLValueAccess : public ParseFunctorBase
{
	RegisterMemberLValueAccess(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		Trace(L"RegisterMemberLValueAccess", str);
		State.RegisterMemberLValueAccess(str);
	}
};

//
// Reset tracking of l-value member accesses
//
struct ResetMemberAccessLValue : public ParseFunctorBase
{
	ResetMemberAccessLValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"ResetMemberAccessLValue", std::wstring(begin, end));
		State.ResetMemberAccessLValue();
	}
};

//
// Register that we are using a member of a composite type as an l-value
//
struct RegisterCompositeLValue : public ParseFunctorBase
{
	RegisterCompositeLValue(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		std::wstring str(begin, end);
		str = str.substr(0, str.find(Operators::Member));
		Trace(L"RegisterCompositeLValue", str);
		State.RegisterCompositeLValue(str);
	}
};

//
// Finish an assignment operation and make sure all the necessary ops are generated
//
struct FinalizeCompositeAssignment : public ParseFunctorBase
{
	FinalizeCompositeAssignment(Parser::ParserState& state)
		: ParseFunctorBase(state)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"FinalizeCompositeAssignment", std::wstring(begin, end));
		State.FinalizeCompositeAssignment();
	}
};

