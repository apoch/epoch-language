//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for Epoch statements
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Forwards.h"
#include "Compiler/Abstract Syntax Tree/Templates.h"


namespace AST
{

	//
	// A regular statement consists of an identifier
	// and a sequence of parameters; this comprises
	// the Epoch function call syntax.
	//
	struct Statement
	{
		IdentifierT Identifier;
		OptionalTemplateArgumentList TemplateArgs;
		std::vector<DeferredExpression, Memory::OneWayAlloc<DeferredExpression> > Params;

		Statement()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		Statement(const Statement&);
		Statement& operator = (const Statement&);
	};

	//
	// A pre-operation statement is an operator followed by
	// an identifier (or a sequence of member accesses)
	//
	struct PreOperatorStatement
	{
		IdentifierT Operator;
		IdentifierList Operand;

		PreOperatorStatement()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		PreOperatorStatement(const PreOperatorStatement&);
		PreOperatorStatement& operator = (const PreOperatorStatement&);
	};

	//
	// A post-operation statement is an identifier (or a
	// sequence of member accesses) followed by an operator
	//
	struct PostOperatorStatement
	{
		IdentifierList Operand;
		IdentifierT Operator;

		PostOperatorStatement()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		PostOperatorStatement(const PostOperatorStatement&);
		PostOperatorStatement& operator = (const PostOperatorStatement&);
	};

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredStatement,
	(AST::IdentifierT, Content->Identifier)
	(AST::OptionalTemplateArgumentList, Content->TemplateArgs)
	(AST::DeferredExpressionVector, Content->Params)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredPreOperatorStatement,
	(AST::IdentifierT, Content->Operator)
	(AST::IdentifierList, Content->Operand)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredPostOperatorStatement,
	(AST::IdentifierList, Content->Operand)
	(AST::IdentifierT, Content->Operator)
)

