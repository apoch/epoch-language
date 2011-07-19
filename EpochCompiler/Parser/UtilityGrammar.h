#pragma once

#include "Compiler/AbstractSyntaxTree.h"
#include "Lexer/Lexer.h"

struct UtilityGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::IdentifierT()>
{
	typedef Lexer::TokenIterT IteratorT;

	explicit UtilityGrammar(const Lexer::EpochLexerT& lexer);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};


	Rule<AST::IdentifierT()>::type StringIdentifier;
};

