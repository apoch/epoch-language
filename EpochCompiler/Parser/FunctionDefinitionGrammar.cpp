#include "pch.h"
#include "Parser/FunctionDefinitionGrammar.h"

#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"

#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"


FunctionDefinitionGrammar::FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const ExpressionGrammar& expressiongrammar)
	: FunctionDefinitionGrammar::base_type(FunctionDefinition)
{
	using namespace boost::spirit::qi;

	ParamTypeSpec = lexer.OpenParens >> -(as<AST::IdentifierT>()[lexer.StringIdentifier] % lexer.Comma) >> lexer.CloseParens;
	ReturnTypeSpec = lexer.OpenParens >> -(lexer.StringIdentifier) >> lexer.CloseParens;
	ParameterFunctionRef = lexer.StringIdentifier >> lexer.Colon >> ParamTypeSpec >> lexer.Arrow >> ReturnTypeSpec;
	ParameterSpec %= lexer.StringIdentifier >> -lexer.Ref >> lexer.OpenParens >> lexer.StringIdentifier >> lexer.CloseParens;
	ParameterDeclaration %= ParameterSpec | ParameterFunctionRef | expressiongrammar;
	EmptyParams = lexer.CloseParens;
	ParameterList %= lexer.OpenParens >> (EmptyParams | ((ParameterDeclaration % lexer.Comma) >> lexer.CloseParens));
	EmptyReturns %= lexer.CloseParens;
	ReturnList %= lexer.OpenParens >> (EmptyReturns | (expressiongrammar >> lexer.CloseParens));

	FunctionTagSpec = (lexer.StringIdentifier >> -(lexer.OpenParens >> ((expressiongrammar) % lexer.Comma) >> lexer.CloseParens));
	FunctionTagList = (lexer.OpenBrace >> *FunctionTagSpec >> lexer.CloseBrace) | omit[eps];

	FunctionDefinition %= lexer.StringIdentifier >> lexer.Colon >> ParameterList >> lexer.Arrow >> ReturnList >> FunctionTagList >> codeblockgrammar;
}
