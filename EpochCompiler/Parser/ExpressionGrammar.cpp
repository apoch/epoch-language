#include "pch.h"

#include "Parser/ExpressionGrammar.h"
#include "Parser/LiteralGrammar.h"
#include "Parser/UtilityGrammar.h"

#include "Lexer/AdaptTokenDirective.h"


ExpressionGrammar::ExpressionGrammar(const Lexer::EpochLexerT& lexer, const LiteralGrammar& literalgrammar, const UtilityGrammar& identifiergrammar)
	: ExpressionGrammar::base_type(Expression)
{
	using namespace boost::spirit::qi;

	InfixSymbols.add(L".");

	MemberAccess %= identifiergrammar % lexer.Dot;

	PreOperatorStatement %= adapttokens[PreOperatorSymbols] >> MemberAccess;
	PostOperatorStatement %= MemberAccess >> adapttokens[PostOperatorSymbols];
	Parenthetical %= lexer.OpenParens >> (PreOperatorStatement | PostOperatorStatement | Expression) >> lexer.CloseParens;

	EntityParamsInner %= (Expression % lexer.Comma) >> lexer.CloseParens;
	EntityParamsEmpty = lexer.CloseParens;
	EntityParams %= lexer.OpenParens >> (EntityParamsEmpty | EntityParamsInner);
	Statement %= identifiergrammar >> EntityParams;

	Prefixes %= *adapttokens[PrefixSymbols];
	ExpressionComponent %= Prefixes >> (Statement | Parenthetical | literalgrammar | identifiergrammar);
	ExpressionFragment %= (adapttokens[InfixSymbols] >> ExpressionComponent);
	Expression %= ExpressionComponent >> *ExpressionFragment;

	AssignmentOperator %= (lexer.Equals | adapttokens[OpAssignSymbols]);
	Assignment %= MemberAccess >> AssignmentOperator >> ExpressionOrAssignment;
	ExpressionOrAssignment %= Assignment | Expression;

	AnyStatement = PreOperatorStatement | PostOperatorStatement | Statement;
}

