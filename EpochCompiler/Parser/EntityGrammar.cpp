#include "pch.h"

#include "Parser/EntityGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/CodeBlockGrammar.h"

#include "Parser/GrammarDebugger.h"

#include "Lexer/AdaptTokenDirective.h"


EntityGrammar::EntityGrammar()
	: EntityGrammar::base_type(AnyEntity)
{
}

void EntityGrammar::InitRecursivePortion(const Lexer::EpochLexerT& lexer, const ExpressionGrammar& expressiongrammar, const CodeBlockGrammar& codeblockgrammar)
{
	using namespace boost::spirit::qi;

	EntityIdentifierMatch = adapttokens[EntityIdentifierSymbols];
	ChainedEntityIdentifierMatch = adapttokens[ChainedEntityIdentifierSymbols];

	ChainedEntity %= ChainedEntityIdentifierMatch >> -expressiongrammar.EntityParams >> codeblockgrammar.InnerCodeBlock;
	Entity %= EntityIdentifierMatch >> -expressiongrammar.EntityParams >> codeblockgrammar.InnerCodeBlock >> *ChainedEntity;
	PostfixEntity %= adapttokens[PostfixEntitySymbols] >> -expressiongrammar.EntityParams >> codeblockgrammar.InnerCodeBlock >> adapttokens[PostfixEntityCloserSymbols] >> expressiongrammar.EntityParams;

	AnyEntity %= Entity | PostfixEntity;

	/*
	GrammarDebugger debugger;
	Entity.name("Entity");
	debug(Entity, debugger);
	PostfixEntity.name("PostfixEntity");
	debug(PostfixEntity, debugger);
	ChainedEntity.name("ChainedEntity");
	debug(ChainedEntity, debugger);
	*/
}

