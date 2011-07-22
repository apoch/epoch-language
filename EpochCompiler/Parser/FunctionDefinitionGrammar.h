#pragma once
#include "Compiler/Abstract Syntax Tree/Function.h"
#include "Lexer/Lexer.h"

struct CodeBlockGrammar;
struct ExpressionGrammar;


struct FunctionDefinitionGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::Deferred<AST::Function>()>
{
	typedef Lexer::TokenIterT IteratorT;

	FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const ExpressionGrammar& expressiongrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::IdentifierList()>::type ParamTypeSpec;
	Rule<AST::IdentifierT()>::type ReturnTypeSpec;
	Rule<AST::DeferredFunctionRefSig()>::type ParameterFunctionRef;
	Rule<AST::DeferredNamedFunctionParameter()>::type ParameterSpec;

	Rule<AST::DeferredFunctionParameter()>::type ParameterDeclaration;
	Rule<boost::spirit::qi::unused_type>::type EmptyReturns;
	Rule<boost::spirit::qi::unused_type>::type EmptyParams;

	Rule<std::vector<AST::DeferredFunctionParameter, Memory::OneWayAlloc<AST::DeferredFunctionParameter> >()>::type ParameterList;
	Rule<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> >()>::type ReturnList;
	Rule<AST::FunctionTagList()>::type FunctionTagList;
	Rule<AST::FunctionTag()>::type FunctionTagSpec;

	Rule<AST::Deferred<AST::Function>()>::type FunctionDefinition;
};

