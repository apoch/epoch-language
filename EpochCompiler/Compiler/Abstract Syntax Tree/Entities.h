//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for entity invocations of all forms
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"


namespace AST
{

	//
	// AST node for a chained entity invocation
	//
	// Chained invocations appear after a normal entity invocation and can be
	// repeated an arbitrary number of times, until ending with an optional
	// termination entity. The canonical example is if/elseif/else.
	//
	struct ChainedEntity
	{
		IdentifierT Identifier;
		DeferredExpressionVector Parameters;
		DeferredCodeBlock Code;

		long RefCount;

		ChainedEntity()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		ChainedEntity(const ChainedEntity&);
		ChainedEntity& operator = (const ChainedEntity&);
	};

	//
	// AST node representing an entity invocation
	//
	// Entities are used to extend flow control and provide other metaprogramming
	// constructs for Epoch programs. The most common use case is to supply syntactic
	// forms for certain idiomatic tasks such as loops, conditionals, exception
	// handlers, and so on. Entities represent a tricky parsing problem but also a
	// fundamentally powerful aspect of Epoch's design.
	//
	struct Entity
	{
		IdentifierT Identifier;
		DeferredExpressionVector Parameters;
		DeferredCodeBlock Code;
		ChainedEntityVector Chain;

		long RefCount;

		Entity()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		Entity(const Entity&);
		Entity& operator = (const Entity&);
	};

	//
	// AST node representing a postfix entity invocation
	//
	// Standard entity invocations take parameters prior to the attached code
	// block, such as if, for, while, and so on. Postfix entities, by contrast,
	// accept parameters after the code block, as in do/while loops.
	//
	struct PostfixEntity
	{
		IdentifierT Identifier;
		DeferredExpressionVector Parameters;
		DeferredCodeBlock Code;
		IdentifierT PostfixIdentifier;
		DeferredExpressionVector PostfixParameters;

		long RefCount;

		PostfixEntity()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		PostfixEntity(const PostfixEntity&);
		PostfixEntity& operator = (const PostfixEntity&);
	};

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredEntity,
	(AST::IdentifierT, Content->Identifier)
	(AST::DeferredExpressionVector, Content->Parameters)
	(AST::DeferredCodeBlock, Content->Code)
	(AST::ChainedEntityVector, Content->Chain)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredPostfixEntity,
	(AST::IdentifierT, Content->Identifier)
	(AST::DeferredExpressionVector, Content->Parameters)
	(AST::DeferredCodeBlock, Content->Code)
	(AST::IdentifierT, Content->PostfixIdentifier)
	(AST::DeferredExpressionVector, Content->PostfixParameters)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredChainedEntity,
	(AST::IdentifierT, Content->Identifier)
	(AST::DeferredExpressionVector, Content->Parameters)
	(AST::DeferredCodeBlock, Content->Code)
)
