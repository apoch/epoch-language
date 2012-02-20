#pragma once

#include "Compiler/Abstract Syntax Tree/CodeBlock.h"

#include "Lexer/Lexer.h"


struct ExpressionGrammar;
struct EntityGrammar;


struct CodeBlockGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::DeferredCodeBlock()>
{
	typedef Lexer::TokenIterT IteratorT;

	CodeBlockGrammar(const Lexer::EpochLexerT& lexer, const ExpressionGrammar& expressiongrammar, const EntityGrammar& entitygrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};


	Rule<boost::spirit::qi::unused_type>::type Empty;
	Rule<AST::Deferred<AST::CodeBlockEntry>()>::type CodeBlockEntry;
	Rule<AST::DeferredCodeBlock()>::type InnerCodeBlock;
	Rule<AST::DeferredCodeBlock()>::type CodeBlock;
};

