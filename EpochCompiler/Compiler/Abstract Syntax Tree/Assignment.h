#pragma once

#include "Compiler/Abstract Syntax Tree/Identifiers.h"


namespace AST
{

	typedef boost::variant
		<
			Undefined,
			DeferredExpression,
			DeferredAssignment
		> ExpressionOrAssignment;


	struct SimpleAssignment
	{
		IdentifierT LHS;
		IdentifierT Operator;
		ExpressionOrAssignment RHS;

		long RefCount;

		SimpleAssignment()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		SimpleAssignment(const SimpleAssignment&);
		SimpleAssignment& operator = (const SimpleAssignment&);
	};

	struct Assignment
	{
		IdentifierList LHS;
		IdentifierT Operator;
		ExpressionOrAssignment RHS;

		long RefCount;

		Assignment()
			: RefCount(0)
		{ }

		Assignment(const DeferredSimpleAssignment& simple)
			: RefCount(0),
			  Operator(simple.Content->Operator),
			  RHS(simple.Content->RHS)
		{
			LHS.Content->Container.push_back(simple.Content->LHS);
		}

	// Non-copyable
	private:
		Assignment(const Assignment&);
		Assignment& operator = (const Assignment&);
	};

}

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredAssignment,
	(AST::IdentifierList, Content->LHS)
	(AST::IdentifierT, Content->Operator)
	(AST::ExpressionOrAssignment, Content->RHS)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredSimpleAssignment,
	(AST::IdentifierT, Content->LHS)
	(AST::IdentifierT, Content->Operator)
	(AST::ExpressionOrAssignment, Content->RHS)
)

