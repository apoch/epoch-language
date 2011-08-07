//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for assignments, including chained assignments and operation-assignments
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"


namespace AST
{

	//
	// Variant describing the right-hand side of an assignment
	//
	// This can be either:
	//		- A terminal expression: foo = 42
	//		- A chained assignment:  foo = bar = 42
	//
	// Both forms are represented with a single AST node type for simplicity. 
	//
	typedef boost::variant
		<
			Undefined,
			DeferredExpression,
			DeferredAssignment
		> ExpressionOrAssignment;


	//
	// A "simple" assignment has a trivial left-hand side
	//
	// We differentiate between simple and full assignment left-hand sides,
	// because we can gain a reasonable bit of performance in the parser by
	// doing so.
	//
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

	//
	// Full assignments can have non-trivial left-hand sides
	//
	// These are measurably more expensive to parse, and reasonably
	// less common in most code than simple assignments, so we keep
	// the two categories distinct.
	//
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

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

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

