#include "pch.h"
#include "Parser/LiteralGrammar.h"


LiteralGrammar::LiteralGrammar()
	: LiteralGrammar::base_type(Literal)
{
	using namespace boost::spirit::qi;

	BooleanLiteral.add
		(L"true", true)
		(L"false", false);

	HexLiteral %= L"0x" >> hex;
	StringLiteral %= L'\"' >> raw[lexeme[*(char_ - L'\"')]] >> L'\"';

	Literal
		%= HexLiteral
		 | Real32Parser
		 | Integer32Parser
		 | StringLiteral
		 | BooleanLiteral
		;
}

