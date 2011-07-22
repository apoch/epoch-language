#pragma once

namespace AST
{

	struct Statement
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > Params;

		Statement()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		Statement(const Statement&);
		Statement& operator = (const Statement&);
	};

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


BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredStatement,
	(AST::IdentifierT, Content->Identifier)
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

