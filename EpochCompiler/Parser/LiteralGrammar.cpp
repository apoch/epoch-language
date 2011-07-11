#include "pch.h"
#include "Parser/LiteralGrammar.h"


LiteralGrammar::LiteralGrammar()
	: LiteralGrammar::base_type(Literal)
{
	using namespace boost::spirit::qi;

	HexLiteral
		= L"0x" >> (+(hex))
		;

	RealLiteral
		= (-char_(L'-')) >> (+(digit)) >> L'.' >> (+(digit))
		;

	IntegerLiteral
		= (-char_(L'-')) >> (+(digit))
		;

	StringLiteral
		= L'\"' >> lexeme[*(char_ - L'\"')] >> L'\"'
		;

	BooleanLiteral
		= lit(L"true")
		| lit(L"false")
		;

	Literal
		= HexLiteral
		| RealLiteral
		| IntegerLiteral
		| StringLiteral
		| BooleanLiteral
		;
}

