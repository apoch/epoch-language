#pragma once

#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"


namespace AST
{

	struct ChainedEntity
	{
		IdentifierT Identifier;
		std::vector<DeferredExpression, Memory::OneWayAlloc<DeferredExpression> > Parameters;
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

	typedef std::vector<AST::DeferredChainedEntity, Memory::OneWayAlloc<AST::DeferredChainedEntity> > ChainedEntityVector;

	struct Entity
	{
		IdentifierT Identifier;
		std::vector<DeferredExpression, Memory::OneWayAlloc<DeferredExpression> > Parameters;
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

	struct PostfixEntity
	{
		IdentifierT Identifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > Parameters;
		DeferredCodeBlock Code;
		IdentifierT PostfixIdentifier;
		std::vector<Deferred<Expression, boost::intrusive_ptr<Expression> >, Memory::OneWayAlloc<Deferred<Expression, boost::intrusive_ptr<Expression> > > > PostfixParameters;

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
