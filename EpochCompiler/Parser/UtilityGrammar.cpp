#include "pch.h"
#include "Parser/UtilityGrammar.h"


UtilityGrammar::UtilityGrammar(const Lexer::EpochLexerT& lexer)
	: UtilityGrammar::base_type(StringIdentifier)
{
	using namespace boost::spirit::qi;

	StringIdentifier %= lexer.StringIdentifier;
}

