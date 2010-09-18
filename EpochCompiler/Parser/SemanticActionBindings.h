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


struct StoreString
{
	StoreString(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
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

struct EndParameterSet
{
	EndParameterSet(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename ParamType>
	void operator () (ParamType) const
	{
		Bindings.EndParameterSet();
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



struct Finalize
{
	Finalize(SemanticActionInterface& bindings)
		: Bindings(bindings)
	{ }

	template <typename IteratorType>
	void operator () (IteratorType begin, IteratorType end) const
	{
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
		Bindings.CompleteAssignment();
	}

	SemanticActionInterface& Bindings;
};