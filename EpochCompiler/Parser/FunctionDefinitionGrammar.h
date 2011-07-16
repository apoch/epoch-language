#pragma once

#include "Parser/SkipGrammar.h"
#include "Compiler/AbstractSyntaxTree.h"

struct CodeBlockGrammar;
struct UtilityGrammar;
struct ExpressionGrammar;


struct FunctionDefinitionGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::Function()>
{
	typedef std::wstring::const_iterator IteratorT;

	FunctionDefinitionGrammar(const CodeBlockGrammar& codeblockgrammar, const UtilityGrammar& identifiergrammar, const ExpressionGrammar& expressiongrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};

	Rule<AST::ParamTypeList()>::type ParamTypeSpec;
	Rule<AST::IdentifierT()>::type ReturnTypeSpec;
	Rule<AST::FunctionReferenceSignature()>::type ParameterFunctionRef;
	Rule<AST::NamedFunctionParameter()>::type ParameterSpec;

	Rule<AST::FunctionParameter()>::type ParameterDeclaration;
	Rule<AST::Deferred<AST::Expression>()>::type ReturnDeclaration;

	Rule<std::list<AST::FunctionParameter>()>::type ParameterList;
	Rule<AST::Deferred<AST::Expression>()>::type ReturnList;
	Rule<AST::FunctionTagList()>::type FunctionTagList;
	Rule<AST::FunctionTag()>::type FunctionTagSpec;

	Rule<AST::Function()>::type FunctionDefinition;
};

