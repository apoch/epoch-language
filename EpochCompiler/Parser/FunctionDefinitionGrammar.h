#pragma once
#include "Compiler/AbstractSyntaxTree.h"
#include "Lexer/Lexer.h"

struct CodeBlockGrammar;
struct ExpressionGrammar;


struct FunctionDefinitionGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::Function()>
{
	typedef Lexer::TokenIterT IteratorT;

	FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const ExpressionGrammar& expressiongrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::ParamTypeList()>::type ParamTypeSpec;
	Rule<AST::IdentifierT()>::type ReturnTypeSpec;
	Rule<DeferredFunctionRefSig()>::type ParameterFunctionRef;
	Rule<AST::NamedFunctionParameter()>::type ParameterSpec;

	Rule<AST::FunctionParameterDeferred()>::type ParameterDeclaration;
	Rule<boost::spirit::qi::unused_type>::type EmptyReturns;
	Rule<boost::spirit::qi::unused_type>::type EmptyParams;

	Rule<std::vector<AST::FunctionParameterDeferred, Memory::OneWayAlloc<AST::FunctionParameterDeferred> >()>::type ParameterList;
	Rule<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> >()>::type ReturnList;
	Rule<AST::FunctionTagList()>::type FunctionTagList;
	Rule<AST::FunctionTag()>::type FunctionTagSpec;

	Rule<AST::Function()>::type FunctionDefinition;
};

