#pragma once

#include "Compiler/Abstract Syntax Tree/Literals.h"
#include "Lexer/Lexer.h"


struct LiteralGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::LiteralToken()>
{
	typedef Lexer::TokenIterT IteratorT;

	explicit LiteralGrammar(const Lexer::EpochLexerT& lexer);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::LiteralToken()>::type Literal;
};

