#pragma once
#include "Compiler/Abstract Syntax Tree/Function.h"
#include "Lexer/Lexer.h"

struct CodeBlockGrammar;
struct ExpressionGrammar;


struct FunctionDefinitionGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::DeferredFunction()>
{
	typedef Lexer::TokenIterT IteratorT;

	FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const ExpressionGrammar& expressiongrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::IdentifierList()>::type ParamTypeSpec;
	Rule<AST::OptionalIdentifier()>::type ReturnTypeSpec;
	Rule<AST::DeferredFunctionRefSig()>::type ParameterFunctionRef;
	Rule<AST::DeferredNamedFunctionParameter()>::type ParameterSpec;

	Rule<AST::RefTag()>::type RefTagRule;

	Rule<AST::DeferredFunctionParameter()>::type ParameterDeclaration;

	Rule<std::vector<AST::DeferredFunctionParameter, Memory::OneWayAlloc<AST::DeferredFunctionParameter> >()>::type ParameterList;
	Rule<AST::OptionalReturn()>::type ReturnList;
	Rule<AST::FunctionTagList()>::type FunctionTagList;
	Rule<AST::FunctionTag()>::type FunctionTagSpec;

	Rule<AST::DeferredFunction()>::type FunctionDefinition;
};

