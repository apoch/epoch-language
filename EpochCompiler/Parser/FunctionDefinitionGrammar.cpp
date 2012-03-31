#include "pch.h"
#include "Parser/FunctionDefinitionGrammar.h"

#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"

#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"


FunctionDefinitionGrammar::FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const ExpressionGrammar& expressiongrammar)
	: FunctionDefinitionGrammar::base_type(FunctionDefinition)
{
	using namespace boost::spirit::qi;

	RefTagRule = as<AST::IdentifierT>()[lexer.Ref];
	ParamTypeSpec = lexer.StringIdentifier % lexer.Comma;
	ReturnTypeSpec %= (lexer.Arrow >> lexer.StringIdentifier) | attr(AST::Undefined());
	ParameterFunctionRef = lexer.OpenParens >> lexer.StringIdentifier >> lexer.Colon >> -ParamTypeSpec >> ReturnTypeSpec >> lexer.CloseParens;
	ParameterSpec %= lexer.StringIdentifier >> -RefTagRule >> lexer.StringIdentifier;
	ParameterDeclaration %= ParameterSpec | ParameterFunctionRef | expressiongrammar;
	ParameterList %= ParameterDeclaration % lexer.Comma;
	ReturnList %= lexer.Arrow >> (expressiongrammar.VariableDeclaration | expressiongrammar);

	FunctionTagSpec = (lexer.StringIdentifier >> -((expressiongrammar) % lexer.Comma));
	FunctionTagList = (lexer.OpenBrace >> *FunctionTagSpec >> lexer.CloseBrace);

	FunctionDefinition %= lexer.StringIdentifier >> lexer.Colon >> -ParameterList >> -ReturnList >> -FunctionTagList >> -codeblockgrammar;
}
