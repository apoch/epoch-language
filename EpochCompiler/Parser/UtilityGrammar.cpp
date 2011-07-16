#include "pch.h"
#include "Parser/UtilityGrammar.h"


UtilityGrammar::UtilityGrammar()
	: UtilityGrammar::base_type(StringIdentifier)
{
	using namespace boost::spirit::qi;

	StringIdentifier %= raw[lexeme[((alpha) >> *(alnum | L'_'))]];
}

