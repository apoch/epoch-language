#include "pch.h"

#include "Parser/ExpressionGrammar.h"
#include "Parser/LiteralGrammar.h"
#include "Parser/UtilityGrammar.h"


ExpressionGrammar::ExpressionGrammar(const LiteralGrammar& literalgrammar, const UtilityGrammar& identifiergrammar)
	: ExpressionGrammar::base_type(Expression)
{
	using namespace boost::spirit::qi;

	InfixIdentifier.add(L".");
	MemberAccess %= identifiergrammar % L'.';

	PreOperatorStatement %= raw[PreOperator] >> MemberAccess;
	PostOperatorStatement %= MemberAccess >> raw[PostOperator];
	Parenthetical %= L'(' >> (PreOperatorStatement | PostOperatorStatement | Expression) >> L')';

	EntityParams %= L'(' >> -(Expression % L',') >> L')';
	Statement %= identifiergrammar >> EntityParams;

	Prefixes = *(raw[UnaryPrefixIdentifier]);
	ExpressionComponent %= Prefixes >> (Statement | Parenthetical | literalgrammar | identifiergrammar);
	ExpressionFragment %= (raw[(InfixIdentifier - PreOperator - PostOperator)] >> ExpressionComponent);
	Expression %= ExpressionComponent >> *ExpressionFragment;

	AssignmentOperator %= raw[(L"=" | raw[OpAssignmentIdentifier])];
	Assignment %= MemberAccess >> raw[AssignmentOperator] >> ExpressionOrAssignment;
	ExpressionOrAssignment %= Assignment | Expression;

	AnyStatement = PreOperatorStatement | PostOperatorStatement | Statement;
}

