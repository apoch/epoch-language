#include "pch.h"
#include "Parser/FunctionDefinitionGrammar.h"

#include "Parser/UtilityGrammar.h"
#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"


FunctionDefinitionGrammar::FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const UtilityGrammar& identifiergrammar, const ExpressionGrammar& expressiongrammar)
	: FunctionDefinitionGrammar::base_type(FunctionDefinition)
{
	using namespace boost::spirit::qi;

	ParamTypeSpec = lexer.OpenParens >> -(identifiergrammar % lexer.Comma) >> lexer.CloseParens;
	ReturnTypeSpec = lexer.OpenParens >> -(identifiergrammar) >> lexer.CloseParens;
	ParameterFunctionRef = identifiergrammar >> lexer.Colon >> ParamTypeSpec >> lexer.Arrow >> ReturnTypeSpec;
	ParameterSpec %= identifiergrammar >> -lexer.Ref >> lexer.OpenParens >> identifiergrammar >> lexer.CloseParens;
	ParameterDeclaration %= ParameterFunctionRef | ParameterSpec | expressiongrammar;
	ParameterList %= lexer.OpenParens >> (-(ParameterDeclaration % lexer.Comma)) >> lexer.CloseParens;
	ReturnDeclaration %= expressiongrammar;
	ReturnList %= lexer.OpenParens >> -ReturnDeclaration >> lexer.CloseParens;

	FunctionTagSpec = (identifiergrammar >> -(lexer.OpenParens >> ((expressiongrammar) % lexer.Comma) >> lexer.CloseParens));
	FunctionTagList = lexer.OpenBrace >> *FunctionTagSpec >> lexer.CloseBrace;

	FunctionDefinition %= identifiergrammar >> lexer.Colon >> ParameterList >> lexer.Arrow >> ReturnList >> -FunctionTagList >> codeblockgrammar;
}
