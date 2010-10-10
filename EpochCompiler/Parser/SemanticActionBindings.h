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

#include "Utility/Files/FilesAndPaths.h"
#include <sstream>

#ifdef _DEBUG
#include <iostream>
#endif

#define PARSER_TRACING

template <typename IteratorType>
void Trace(const std::wstring& title, IteratorType begin, IteratorType end)
{
#ifdef _DEBUG
#ifdef PARSER_TRACING
	std::wstring blob(begin, end);
	std::wcout << L"PARSER TRACE: " << title << L" - " << blob << std::endl;
#endif
#endif
}

void Trace(const std::wstring& title)
{
#ifdef _DEBUG
#ifdef PARSER_TRACING
	std::wcout << L"PARSER TRACE: " << title << std::endl;
#endif
#endif
}


struct GeneralExceptionHandler
{
	explicit GeneralExceptionHandler(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ScannerType, typename ErrorType>
	boost::spirit::classic::error_status<> operator () (const ScannerType& thescanner, const ErrorType& theerror) const
	{
		boost::spirit::classic::file_position pos = theerror.where.get_position();

		std::wcout << L"Error in file \"" << StripPath(widen(pos.file)) << L"\" on line " << pos.line << L":\n";
		std::wcout << theerror.descriptor.what() << std::endl << std::endl;

        // Move past the broken line and continue parsing
        while(thescanner.first.get_position().line <= pos.line)
                ++thescanner;

		return boost::spirit::classic::error_status<>(boost::spirit::classic::error_status<>::retry);
	}

	SemanticActionInterface& Bindings;
};

struct MissingFunctionBodyExceptionHandler
{
	explicit MissingFunctionBodyExceptionHandler(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ScannerType, typename ErrorType>
	boost::spirit::classic::error_status<> operator () (const ScannerType& thescanner, const ErrorType& theerror) const
	{
		Bindings.EmitPendingCode();
		Bindings.StoreEntityCode();
		return boost::spirit::classic::error_status<>(boost::spirit::classic::error_status<>::accept, 1);
	}

	SemanticActionInterface& Bindings;
};


struct StoreString
{
	explicit StoreString(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreString", begin, end);
		Bindings.SetParsePosition(end);

		std::wstring str(begin, end);
		Bindings.StoreString(str);
	}

	SemanticActionInterface& Bindings;
};

struct StoreIntegerLiteral
{
	explicit StoreIntegerLiteral(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreIntegerLiteral", begin, end);
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
	explicit StoreStringLiteral(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreStringLiteral", begin, end);
		Bindings.SetParsePosition(end);

		std::wstring str(begin, end);
		Bindings.StoreStringLiteral(str.substr(1, str.length() - 2));
	}

	SemanticActionInterface& Bindings;
};

struct StoreBooleanLiteral
{
	explicit StoreBooleanLiteral(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreBooleanLiteral", begin, end);
		Bindings.SetParsePosition(end);

		std::wstring str(begin, end);
		if(str == L"true")
			Bindings.StoreBooleanLiteral(true);
		else if(str == L"false")
			Bindings.StoreBooleanLiteral(false);
		else
			throw FatalException("Invalid boolean literal");
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
		Trace(L"StoreEntityType", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.StoreEntityType(TypeTag);
	}

	SemanticActionInterface& Bindings;
	Bytecode::EntityTag TypeTag;
};

struct StoreEntityTypeByString
{
	explicit StoreEntityTypeByString(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreEntityTypeByString", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.StoreEntityType(std::wstring(begin, end));
	}

	SemanticActionInterface& Bindings;
};

struct StoreEntityPostfixByString
{
	explicit StoreEntityPostfixByString(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreEntityPostfixByString", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.StoreEntityPostfix(std::wstring(begin, end));
	}

	SemanticActionInterface& Bindings;
};

struct StoreEntityCode
{
	explicit StoreEntityCode(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreEntityCode", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.StoreEntityCode();
	}

	SemanticActionInterface& Bindings;
};

struct BeginEntityChain
{
	explicit BeginEntityChain(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"BeginEntityChain", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.BeginEntityChain();
	}

	SemanticActionInterface& Bindings;
};

struct EndEntityChain
{
	explicit EndEntityChain(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"EndEntityChain", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.EndEntityChain();
	}

	SemanticActionInterface& Bindings;
};

struct InvokePostfixMetacontrol
{
	explicit InvokePostfixMetacontrol(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"InvokePostfixMetacontrol");
		Bindings.InvokePostfixMetacontrol();
	}

	SemanticActionInterface& Bindings;
};

struct StoreInfix
{
	explicit StoreInfix(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreInfix", begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.StoreInfix(str);
	}

	SemanticActionInterface& Bindings;
};

struct CompleteInfix
{
	explicit CompleteInfix(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"CompleteInfix", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.CompleteInfix();
	}

	SemanticActionInterface& Bindings;
};

struct FinalizeInfix
{
	explicit FinalizeInfix(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"FinalizeInfix", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.FinalizeInfix();
	}

	SemanticActionInterface& Bindings;
};

struct StoreUnaryPrefixOperator
{
	explicit StoreUnaryPrefixOperator(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"StoreUnaryPrefixOperator", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.StoreUnaryPrefixOperator(std::wstring(begin, end));
	}

	SemanticActionInterface& Bindings;
};

struct BeginParameterSet
{
	explicit BeginParameterSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginParameterSet");
		Bindings.BeginParameterSet();
	}

	SemanticActionInterface& Bindings;
};

struct EndParameterSet
{
	explicit EndParameterSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EndParameterSet");
		Bindings.EndParameterSet();
	}

	SemanticActionInterface& Bindings;
};

struct RegisterParameterType
{
	explicit RegisterParameterType(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterParameterType", begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterParameterType(str);
	}

	SemanticActionInterface& Bindings;
};

struct RegisterParameterName
{
	explicit RegisterParameterName(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterParameterName", begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterParameterName(str);
	}

	SemanticActionInterface& Bindings;
};

struct BeginReturnSet
{
	explicit BeginReturnSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginReturnSet");
		Bindings.BeginReturnSet();
	}

	SemanticActionInterface& Bindings;
};

struct EndReturnSet
{
	explicit EndReturnSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EndReturnSet");
		Bindings.EndReturnSet();
	}

	SemanticActionInterface& Bindings;
};

struct RegisterReturnType
{
	explicit RegisterReturnType(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterReturnType", begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterReturnType(str);
	}

	SemanticActionInterface& Bindings;
};

struct RegisterReturnName
{
	explicit RegisterReturnName(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterReturnName", begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.RegisterReturnName(str);
	}

	SemanticActionInterface& Bindings;
};

struct RegisterReturnValue
{
	explicit RegisterReturnValue(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterReturnValue", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.RegisterReturnValue();
	}

	SemanticActionInterface& Bindings;
};

struct BeginStatement
{
	explicit BeginStatement(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"BeginStatement", begin, end);
		Bindings.SetParsePosition(end);
		std::wstring str(begin, end);
		Bindings.BeginStatement(str);
	}

	SemanticActionInterface& Bindings;
};

struct BeginStatementParams
{
	explicit BeginStatementParams(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginStatementParams");
		Bindings.BeginStatementParams();
	}

	SemanticActionInterface& Bindings;
};

struct BeginEntityParams
{
	explicit BeginEntityParams(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"BeginEntityParams");
		Bindings.BeginEntityParams();
	}

	SemanticActionInterface& Bindings;
};

struct PushStatementParam
{
	explicit PushStatementParam(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushStatementParam", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.PushStatementParam();
	}

	SemanticActionInterface& Bindings;
};

struct PushInfixParam
{
	explicit PushInfixParam(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"PushInfixParam", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.PushInfixParam();
	}

	SemanticActionInterface& Bindings;
};

struct CompleteStatement
{
	explicit CompleteStatement(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"CompleteStatement");
		Bindings.CompleteStatement();
	}

	SemanticActionInterface& Bindings;
};

struct CompleteEntityParams
{
	CompleteEntityParams(SemanticActionInterface& bindings, bool ispostfixcloser)
		: Bindings(bindings),
		  IsPostfixCloser(ispostfixcloser)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"CompleteEntityParams");
		Bindings.CompleteEntityParams(IsPostfixCloser);
	}

	SemanticActionInterface& Bindings;
	bool IsPostfixCloser;
};

struct FinalizeStatement
{
	explicit FinalizeStatement(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"FinalizeStatement", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.FinalizeStatement();
	}

	SemanticActionInterface& Bindings;
};

struct BeginParenthetical
{
	explicit BeginParenthetical(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginParenthetical");
		Bindings.BeginParenthetical();
	}

	SemanticActionInterface& Bindings;
};

struct EndParenthetical
{
	explicit EndParenthetical(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"EndParenthetical", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.EndParenthetical();
	}

	SemanticActionInterface& Bindings;
};


struct BeginLexicalScope
{
	explicit BeginLexicalScope(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"BeginLexicalScope");
		Bindings.StoreEntityType(Bytecode::EntityTags::FreeBlock);
	}

	SemanticActionInterface& Bindings;
};

struct EndLexicalScope
{
	explicit EndLexicalScope(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EndLexicalScope");
		Bindings.StoreEntityCode();
	}

	SemanticActionInterface& Bindings;
};


struct Finalize
{
	explicit Finalize(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"Finalize", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.Finalize();
	}

	SemanticActionInterface& Bindings;
};


struct EmitPendingCode
{
	explicit EmitPendingCode(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Trace(L"EmitPendingCode");
		Bindings.EmitPendingCode();
	}

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"EmitPendingCode", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.EmitPendingCode();
	}

	SemanticActionInterface& Bindings;
};


struct BeginAssignment
{
	explicit BeginAssignment(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"BeginAssignment", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.BeginAssignment();
	}

	SemanticActionInterface& Bindings;
};

struct BeginOpAssignment
{
	explicit BeginOpAssignment(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"BeginOpAssignment", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.BeginOpAssignment(std::wstring(begin, end));
	}

	SemanticActionInterface& Bindings;
};

struct CompleteAssignment
{
	explicit CompleteAssignment(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"CompleteAssignment", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.CompleteAssignment();
	}

	SemanticActionInterface& Bindings;
};


struct RegisterPatternMatchedParameter
{
	explicit RegisterPatternMatchedParameter(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
		Trace(L"RegisterPatternMatchedParameter", begin, end);
		Bindings.SetParsePosition(end);
		Bindings.RegisterPatternMatchedParameter();
	}

	SemanticActionInterface& Bindings;
};
