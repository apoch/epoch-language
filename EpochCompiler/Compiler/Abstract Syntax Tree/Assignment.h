//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for assignments, including chained assignments and operation-assignments
//
// This also includes node definitions for initializations, since initializers arguably
// "look" like assignments in the syntax, even though they are compiled as statements.
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Templates.h"


namespace AST
{

	//
	// Variant describing the right-hand side of an assignment
	//
	// This can be either:
	//		- A terminal expression, as in: foo = 42
	//		- A chained assignment, as in:  foo = bar = 42
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
	// A "simple" assignment has a trivial left-hand side, i.e.
	// not a structure member access or anything of that nature
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
			: Operator(simple.Content->Operator),
			  RHS(simple.Content->RHS), RefCount(0)
		{
			LHS.Content->Container.push_back(simple.Content->LHS);
		}

	// Non-copyable
	private:
		Assignment(const Assignment&);
		Assignment& operator = (const Assignment&);
	};

	//
	// Variable initializations are like simple assignments
	// except they include a type specifier. The operator
	// is also always simply = and not some other operator.
	//
	struct Initialization
	{
		IdentifierT TypeSpecifier;
		OptionalTemplateArgumentList TemplateArgs;
		IdentifierT LHS;
		DeferredExpressionVector RHS;

		long RefCount;

		Initialization()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		Initialization(const Initialization&);
		Initialization& operator = (const Initialization&);
	};

	//
	// An optional initialization might be undefined
	//
	typedef boost::variant
		<
			Undefined,
			DeferredInitialization
		> OptionalInitialization;

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

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredInitialization,
	(AST::IdentifierT, Content->TypeSpecifier)
	(AST::OptionalTemplateArgumentList, Content->TemplateArgs)
	(AST::IdentifierT, Content->LHS)
	(AST::DeferredExpressionVector, Content->RHS)
)

