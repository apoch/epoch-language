#include "pch.h"

#include "Parser/ExpressionGrammar.h"
#include "Parser/LiteralGrammar.h"

#include "Lexer/AdaptTokenDirective.h"

#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Function.h"


ExpressionGrammar::ExpressionGrammar(const Lexer::EpochLexerT& lexer, const LiteralGrammar& literalgrammar)
	: ExpressionGrammar::base_type(Expression)
{
	using namespace boost::spirit::qi;

	InfixSymbols.add(L".");

	MemberAccess %= lexer.StringIdentifier % lexer.Dot;

	PreOperatorStatement %= adapttokens[PreOperatorSymbols] >> MemberAccess;
	PostOperatorStatement %= MemberAccess >> adapttokens[PostOperatorSymbols];
	Parenthetical %= lexer.OpenParens >> (PreOperatorStatement | PostOperatorStatement | Expression) >> lexer.CloseParens;

	TemplateArguments %= lexer.OpenAngleBracket >> ((literalgrammar | as<AST::IdentifierT>()[lexer.StringIdentifier] | as<AST::IdentifierT>()[lexer.Nothing]) % lexer.Comma) >> lexer.CloseAngleBracket;

	EntityParamsInner %= (Expression % lexer.Comma) >> lexer.CloseParens;
	EntityParamsEmpty = lexer.CloseParens;
	EntityParams %= lexer.OpenParens >> (EntityParamsEmpty | EntityParamsInner);
	Statement %= lexer.StringIdentifier >> -TemplateArguments >> EntityParams;

	Prefix %= adapttokens[PrefixSymbols];
	Prefixes %= +Prefix;
	ExpressionChunk = (Parenthetical | literalgrammar | Statement | as<AST::IdentifierT>()[lexer.StringIdentifier] | as<AST::IdentifierT>()[lexer.Nothing]);
	ExpressionComponent %= ExpressionChunk | (Prefixes >> ExpressionChunk);
	ExpressionFragment %= (adapttokens[InfixSymbols] >> ExpressionComponent);
	Expression %= ExpressionComponent >> *ExpressionFragment;

	AssignmentOperator %= (as<AST::IdentifierT>()[lexer.Equals] | adapttokens[OpAssignSymbols]);
	SimpleAssignment %= (as<AST::IdentifierT>()[lexer.StringIdentifier] >> AssignmentOperator >> ExpressionOrAssignment);
	MemberAssignment %= (MemberAccess >> AssignmentOperator >> ExpressionOrAssignment);
	Assignment %= SimpleAssignment | MemberAssignment;
	ExpressionOrAssignment %= Assignment | Expression;

	VariableDeclaration %=
		as<AST::IdentifierT>()[lexer.StringIdentifier]		>>
		-TemplateArguments									>>
		as<AST::IdentifierT>()[lexer.StringIdentifier]		>>
		omit[lexer.Equals]									>>
		(Expression % lexer.Comma)							;

	AnyStatement = PreOperatorStatement | Statement | PostOperatorStatement | VariableDeclaration;
}

