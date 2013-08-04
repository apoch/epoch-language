#include "pch.h"
#include "Parser/FunctionDefinitionGrammar.h"

#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/LiteralGrammar.h"

#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"


FunctionDefinitionGrammar::FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const ExpressionGrammar& expressiongrammar, const LiteralGrammar& literalgrammar)
	: FunctionDefinitionGrammar::base_type(FunctionDefinition)
{
	using namespace boost::spirit::qi;

	TemplateArguments %= lexer.OpenAngleBracket >> ((literalgrammar | as<AST::IdentifierT>()[lexer.StringIdentifier] | as<AST::IdentifierT>()[lexer.Nothing]) % lexer.Comma) >> lexer.CloseAngleBracket;
	TemplateParameter %= (lexer.StringIdentifier | (as<AST::IdentifierT>()[lexer.TypeDef])) >> lexer.StringIdentifier;
	TemplateParameterList %= lexer.OpenAngleBracket >> (TemplateParameter % lexer.Comma) >> lexer.CloseAngleBracket;

	RefTagRule = as<AST::IdentifierT>()[lexer.Ref];
	Nothing = as<AST::IdentifierT>()[lexer.Nothing];
	ParamTypeSpec = lexer.StringIdentifier % lexer.Comma;
	ReturnTypeSpec %= (lexer.Arrow >> lexer.StringIdentifier) | attr(AST::Undefined());
	ParameterFunctionRef = lexer.OpenParens >> lexer.StringIdentifier >> lexer.Colon >> -ParamTypeSpec >> ReturnTypeSpec >> lexer.CloseParens;
	ParameterSpec %= lexer.StringIdentifier >> -TemplateArguments >> -RefTagRule >> lexer.StringIdentifier;
	ParameterDeclaration %= Nothing | ParameterSpec | ParameterFunctionRef | expressiongrammar;
	ParameterList %= ParameterDeclaration % lexer.Comma;
	ReturnList %= lexer.Arrow >> (expressiongrammar.VariableDeclaration | expressiongrammar);
	FunctionTagSpec = (lexer.StringIdentifier >> -(lexer.OpenParens >> ((literalgrammar) % lexer.Comma) >> lexer.CloseParens));
	FunctionTagList = (lexer.OpenBrace >> *FunctionTagSpec >> lexer.CloseBrace);

	FunctionDefinition %= lexer.StringIdentifier >> -TemplateParameterList >> lexer.Colon >> -ParameterList >> -ReturnList >> -FunctionTagList >> -codeblockgrammar;
}
