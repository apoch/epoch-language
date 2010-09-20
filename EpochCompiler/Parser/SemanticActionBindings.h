//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Functors for binding the parser grammar's semantic actions
// to the actual semantic action interface we use internally.
//

#pragma once


// Dependencies
#include "Parser/SemanticActionInterface.h"

#include "Bytecode/EntityTags.h"

#include <sstream>

#ifdef _DEBUG
#include <iostream>
#endif


template <typename IteratorType>
void Trace(IteratorType begin, IteratorType end)
{
#ifdef _DEBUG
	//std::wstring blob(begin, end);
	//std::wcout << L"PARSER TRACE: " << blob << std::endl;
#endif
}


struct GeneralExceptionHandler
{
	GeneralExceptionHandler(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ScannerType, typename ErrorType>
	boost::spirit::classic::error_status<> operator () (const ScannerType& thescanner, const ErrorType& theerror) const
	{
		boost::spirit::classic::file_position pos = theerror.where.get_position();

		std::wcout << L"Error in file \"" << widen(pos.file) << L"\" on line " << pos.line << L":\n";
		std::wcout << theerror.descriptor.what() << std::endl << std::endl;

        // Move past the broken line and continue parsing
        while(thescanner.first.get_position().line <= pos.line)
                ++thescanner;

		return boost::spirit::classic::error_status<>(boost::spirit::classic::error_status<>::retry);
	}

	SemanticActionInterface& Bindings;
};


struct StoreString
{
	StoreString(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);

		std::wstring str(begin, end);
		Bindings.StoreString(str);
	}

	SemanticActionInterface& Bindings;
};

struct StoreIntegerLiteral
{
	StoreIntegerLiteral(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);

		std::wstring str(begin, end);
		Integer32 value;

		std::wstringstream conversion;
		conversion << str;
		conversion >> value;

		Bindings.StoreIntegerLiteral(value);
	}

	SemanticActionInterface& Bindings;
};

struct StoreStringLiteral
{
	StoreStringLiteral(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);

		std::wstring str(begin, end);
		Bindings.StoreStringLiteral(str.substr(1, str.length() - 2));
	}

	SemanticActionInterface& Bindings;
};


struct StoreEntityType
{
	StoreEntityType(SemanticActionInterface& bindings, Bytecode::EntityTag typetag)
		: Bindings(bindings),
		  TypeTag(typetag)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.StoreEntityType(TypeTag);
	}

	SemanticActionInterface& Bindings;
	Bytecode::EntityTag TypeTag;
};

struct StoreEntityCode
{
	StoreEntityCode(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.StoreEntityCode();
	}

	SemanticActionInterface& Bindings;
};

struct StoreInfix
{
	StoreInfix(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.StoreInfix(str);
	}

	SemanticActionInterface& Bindings;
};

struct CompleteInfix
{
	CompleteInfix(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.CompleteInfix();
	}

	SemanticActionInterface& Bindings;
};

struct BeginParameterSet
{
	BeginParameterSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Bindings.BeginParameterSet();
	}

	SemanticActionInterface& Bindings;
};

struct RegisterParameterType
{
	RegisterParameterType(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterParameterType(str);
	}

	SemanticActionInterface& Bindings;
};

struct RegisterParameterName
{
	RegisterParameterName(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterParameterName(str);
	}

	SemanticActionInterface& Bindings;
};

struct BeginReturnSet
{
	BeginReturnSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Bindings.BeginReturnSet();
	}

	SemanticActionInterface& Bindings;
};

struct EndReturnSet
{
	EndReturnSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Bindings.EndReturnSet();
	}

	SemanticActionInterface& Bindings;
};

struct RegisterReturnType
{
	RegisterReturnType(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterReturnType(str);
	}

	SemanticActionInterface& Bindings;
};

struct RegisterReturnName
{
	RegisterReturnName(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterReturnName(str);
	}

	SemanticActionInterface& Bindings;
};

struct RegisterReturnValue
{
	RegisterReturnValue(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.RegisterReturnValue();
	}

	SemanticActionInterface& Bindings;
};

struct BeginStatement
{
	BeginStatement(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.BeginStatement(str);
	}

	SemanticActionInterface& Bindings;
};

struct BeginStatementParams
{
	BeginStatementParams(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Bindings.BeginStatementParams();
	}

	SemanticActionInterface& Bindings;
};

struct ValidateStatementParam
{
	ValidateStatementParam(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.ValidateStatementParam();
	}

	SemanticActionInterface& Bindings;
};

struct CompleteStatement
{
	CompleteStatement(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Bindings.CompleteStatement();
	}

	SemanticActionInterface& Bindings;
};

struct FinalizeStatement
{
	FinalizeStatement(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.FinalizeStatement();
	}

	SemanticActionInterface& Bindings;
};


struct Finalize
{
	Finalize(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.Finalize();
	}

	SemanticActionInterface& Bindings;
};


struct EmitPendingCode
{
	EmitPendingCode(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Bindings.EmitPendingCode();
	}

	SemanticActionInterface& Bindings;
};


struct BeginAssignment
{
	BeginAssignment(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.BeginAssignment();
	}

	SemanticActionInterface& Bindings;
};

struct CompleteAssignment
{
	CompleteAssignment(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(begin, end);
		Bindings.SetParsePosition(end);
		Bindings.CompleteAssignment();
	}

	SemanticActionInterface& Bindings;
};

