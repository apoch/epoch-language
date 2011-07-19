#include "pch.h"
#include "Parser/LiteralGrammar.h"


boost::spirit::qi::rule<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, boost::spirit::qi::unused_type> ConsumeAnything;


LiteralGrammar::LiteralGrammar(const Lexer::EpochLexerT& lexer)
	: LiteralGrammar::base_type(Literal)
{
	using namespace boost::spirit::qi;

	Literal = lexer.StringLiteral | lexer.HexLiteral | lexer.RealLiteral | lexer.IntegerLiteral;
}

