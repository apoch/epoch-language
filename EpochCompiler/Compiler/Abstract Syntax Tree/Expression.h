#pragma once

#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Literals.h"
#include "Compiler/Abstract Syntax Tree/Parenthetical.h"

namespace AST
{
	typedef boost::variant
		<
			Undefined,
			IdentifierT,
			LiteralToken,
			Deferred<struct Statement, boost::intrusive_ptr<Statement> >,
			Parenthetical
		> ExpressionComponentInternalVariant;

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

	struct Expression
	{
		DeferredExpressionComponent First;
		std::vector<DeferredExpressionFragment, Memory::OneWayAlloc<Deferred<ExpressionFragment, boost::intrusive_ptr<ExpressionFragment> > > > Remaining;

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

}


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

