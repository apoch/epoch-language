#pragma once

#include "Compiler/Abstract Syntax Tree/Entities.h"

#include "Lexer/Lexer.h"


struct ExpressionGrammar;
struct CodeBlockGrammar;


struct EntityGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::AnyEntity()>
{
	typedef Lexer::TokenIterT IteratorT;

	EntityGrammar();

	void InitRecursivePortion(const ExpressionGrammar& expressiongrammar, const CodeBlockGrammar& codeblockgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::DeferredEntity()>::type Entity;
	Rule<AST::ChainedEntityVector()>::type ChainedEntities;
	Rule<AST::DeferredPostfixEntity()>::type PostfixEntity;

	Rule<AST::AnyEntity()>::type AnyEntity;

	Rule<AST::IdentifierT()>::type EntityIdentifierMatch;
	Rule<AST::IdentifierT()>::type ChainedEntityIdentifierMatch;

	typedef boost::spirit::qi::symbols<wchar_t, AST::IdentifierT> SymbolTable;
	SymbolTable EntityIdentifierSymbols;
	SymbolTable ChainedEntityIdentifierSymbols;
	SymbolTable PostfixEntitySymbols;
	SymbolTable PostfixEntityCloserSymbols;
};

