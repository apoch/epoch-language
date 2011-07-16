#include "pch.h"

#include "Parser/EntityGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/CodeBlockGrammar.h"


EntityGrammar::EntityGrammar()
	: EntityGrammar::base_type(AnyEntity)
{
}

void EntityGrammar::InitRecursivePortion(const ExpressionGrammar& expressiongrammar, const CodeBlockGrammar& codeblockgrammar)
{
	using namespace boost::spirit::qi;

	ChainedEntity %= raw[ChainedEntityIdentifier] >> -expressiongrammar.EntityParams >> codeblockgrammar.InnerCodeBlock;
	Entity %= raw[EntityIdentifier] >> -expressiongrammar.EntityParams >> codeblockgrammar.InnerCodeBlock >> *ChainedEntity;
	PostfixEntity %= raw[PostfixEntityOpenerIdentifier] >> -expressiongrammar.EntityParams >> codeblockgrammar.InnerCodeBlock >> raw[PostfixEntityCloserIdentifier] >> expressiongrammar.EntityParams;

	AnyEntity %= Entity | PostfixEntity;
}

