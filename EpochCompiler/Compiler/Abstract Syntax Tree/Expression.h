//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for expressions and their constituent parts
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Literals.h"
#include "Compiler/Abstract Syntax Tree/Parenthetical.h"


namespace AST
{
	//
	// Variant describing a component of an expression
	//
	// This can consist of:
	//		- A lone identifier, such as a variable
	//		- A literal value
	//		- A statement, i.e. foo(params)
	//		- A parenthetical expression
	//
	typedef boost::variant
		<
			Undefined,
			IdentifierT,
			LiteralToken,
			DeferredStatement,
			Parenthetical
		> ExpressionComponentInternalVariant;

	//
	// Refcountable wrapper for an expression component variant
	//
	// This is ugly but has shown measurable performance benefits.
	//
	struct ExpressionComponentInternal
	{
		ExpressionComponentInternalVariant V;

		long RefCount;

		ExpressionComponentInternal()
			: RefCount(0)
		{ }

		template <typename T>
		ExpressionComponentInternal(const T& v)
			: V(v),
			  RefCount(0)
		{
		}

	// Non-copyable
	private:
		ExpressionComponentInternal(const ExpressionComponentInternal&);
		ExpressionComponentInternal& operator = (const ExpressionComponentInternal&);
	};

	//
	// AST node representing a component of an expression
	//
	// A component consists of one of the above variants, coupled with
	// an optional set of unary prefix operators that are applied to
	// the component term itself.
	//
	struct ExpressionComponent
	{
		typedef Deferred<ExpressionComponentInternal, boost::intrusive_ptr<ExpressionComponentInternal> > DeferredInternal;

		IdentifierList UnaryPrefixes;
		DeferredInternal Component;

		long RefCount;

		ExpressionComponent()
			: RefCount(0)
		{ }

		ExpressionComponent(const LiteralToken& literaltoken)
			: Component(literaltoken),
			  RefCount(0)
		{ }

		ExpressionComponent(const DeferredInternal& definternal)
			: Component(definternal),
			  RefCount(0)
		{ }

	// Non-copyable
	private:
		ExpressionComponent(const ExpressionComponent&);
		ExpressionComponent& operator = (const ExpressionComponent&);
	};

	//
	// Fragment of an expression
	//
	// A fragment joins an infix operator with the term on the right-hand
	// side of the operation, which is an expression component
	//
	struct ExpressionFragment
	{
		IdentifierT Operator;
		DeferredExpressionComponent Component;

		ExpressionFragment()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		ExpressionFragment(const ExpressionFragment&);
		ExpressionFragment& operator = (const ExpressionFragment&);
	};

	//
	// An expression consists of a component followed by
	// zero or more expression fragments
	//
	struct Expression
	{
		DeferredExpressionComponent First;
		std::vector<DeferredExpressionFragment, Memory::OneWayAlloc<DeferredExpressionFragment> > Remaining;

		long RefCount;

		Expression()
			: RefCount(0)
		{ }

		Expression(const LiteralToken& token)
			: First(token),
			  RefCount(0)
		{ }

	// Non-copyable
	private:
		Expression(const Expression&);
		Expression& operator = (const Expression&);
	};


	//
	// An optional expression might be undefined
	//
	typedef boost::variant
		<
			Undefined,
			DeferredExpression
		> OptionalExpression;



	typedef boost::variant
		<
			Undefined,
			IdentifierT,
			LiteralToken
		> TemplateArgument;


	typedef std::vector<TemplateArgument> TemplateArgumentList;


	typedef boost::variant
		<
			Undefined,
			TemplateArgumentList
		> OptionalTemplateArgumentList;

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::ExpressionComponent::DeferredInternal,
	(AST::ExpressionComponentInternalVariant, Content->V)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredExpressionComponent,
	(AST::IdentifierList, Content->UnaryPrefixes)
	(AST::ExpressionComponent::DeferredInternal, Content->Component)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredExpressionFragment,
	(AST::IdentifierT, Content->Operator)
	(AST::DeferredExpressionComponent, Content->Component)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredExpression,
	(AST::DeferredExpressionComponent, Content->First)
	(AST::DeferredExpressionFragmentVector, Content->Remaining)
)

