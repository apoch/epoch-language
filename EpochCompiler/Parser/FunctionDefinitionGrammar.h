#pragma once
#include "Compiler/AbstractSyntaxTree.h"
#include "Lexer/Lexer.h"

struct CodeBlockGrammar;
struct UtilityGrammar;
struct ExpressionGrammar;


struct FunctionDefinitionGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::Function()>
{
	typedef Lexer::TokenIterT IteratorT;

	FunctionDefinitionGrammar(const Lexer::EpochLexerT& lexer, const CodeBlockGrammar& codeblockgrammar, const UtilityGrammar& identifiergrammar, const ExpressionGrammar& expressiongrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::ParamTypeList()>::type ParamTypeSpec;
	Rule<AST::IdentifierT()>::type ReturnTypeSpec;
	Rule<AST::FunctionReferenceSignature()>::type ParameterFunctionRef;
	Rule<AST::NamedFunctionParameter()>::type ParameterSpec;

	Rule<AST::FunctionParameter()>::type ParameterDeclaration;
	Rule<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> >()>::type ReturnDeclaration;

	Rule<std::vector<AST::FunctionParameter>()>::type ParameterList;
	Rule<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> >()>::type ReturnList;
	Rule<AST::FunctionTagList()>::type FunctionTagList;
	Rule<AST::FunctionTag()>::type FunctionTagSpec;

	Rule<AST::Function()>::type FunctionDefinition;
};

