#pragma once

#include "Compiler/AbstractSyntaxTree.h"

#include "Lexer/Lexer.h"


struct ExpressionGrammar;
struct CodeBlockGrammar;


struct EntityGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::AnyEntity()>
{
	typedef Lexer::TokenIterT IteratorT;

	EntityGrammar();

	void InitRecursivePortion(const Lexer::EpochLexerT& lexer, const ExpressionGrammar& expressiongrammar, const CodeBlockGrammar& codeblockgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::Deferred<AST::Entity>()>::type Entity;
	Rule<std::vector<AST::ChainedEntityDeferred, Memory::OneWayAlloc<AST::ChainedEntityDeferred> >()>::type ChainedEntities;
	Rule<DeferredPostfixEntity()>::type PostfixEntity;

	Rule<AST::AnyEntity()>::type AnyEntity;

	Rule<AST::IdentifierT()>::type EntityIdentifierMatch;
	Rule<AST::IdentifierT()>::type ChainedEntityIdentifierMatch;

	typedef boost::spirit::qi::symbols<wchar_t, AST::IdentifierT> SymbolTable;
	SymbolTable EntityIdentifierSymbols;
	SymbolTable ChainedEntityIdentifierSymbols;
	SymbolTable PostfixEntitySymbols;
	SymbolTable PostfixEntityCloserSymbols;
};

