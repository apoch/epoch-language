//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Definitions for informing boost::variant that various AST node
// constructors will never throw exceptions. This permits the variants
// containing those node types to avoid excess heap allocations and
// generally optimizes the behavior of the variants.
//
// See http://www.boost.org/doc/libs/1_47_0/doc/html/variant/design.html
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Undefined.h"


namespace boost
{

	template <>
	struct has_nothrow_constructor<AST::Undefined>
		: mpl::true_
	{
	};


	template <>
	struct has_nothrow_copy<AST::Undefined>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredPreOperatorStatement>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredPostOperatorStatement>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredStatement>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredExpression>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredAssignment>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredSimpleAssignment>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredEntity>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredPostfixEntity>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredCodeBlock>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredExpressionComponent>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredExpressionFragment>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredFunctionParameter>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredFunctionRefSig>
		: mpl::true_
	{
	};

	template <>
	struct has_nothrow_copy<AST::DeferredNamedFunctionParameter>
		: mpl::true_
	{
	};

}

