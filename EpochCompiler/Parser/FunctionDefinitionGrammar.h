#pragma once

#include "Parser/SkipGrammar.h"
#include "Compiler/AbstractSyntaxTree.h"

struct CodeBlockGrammar;
struct UtilityGrammar;


struct FunctionDefinitionGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::Function()>
{
	typedef std::wstring::const_iterator IteratorT;

	FunctionDefinitionGrammar(const CodeBlockGrammar& codeblockgrammar, const UtilityGrammar& identifiergrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};

	typedef Rule<boost::spirit::qi::unused_type>::type RuleType;

	RuleType ParamTypeSpec;
	RuleType ReturnTypeSpec;
	RuleType ParameterFunctionRef;
	Rule<AST::FunctionParameter()>::type ParameterSpec;

	Rule<AST::FunctionParameter()>::type ParameterDeclaration;
	RuleType ReturnDeclaration;

	Rule<std::vector<AST::FunctionParameter>()>::type ParameterList;
	RuleType ReturnList;
	RuleType FunctionTagList;
	RuleType FunctionTagSpec;

	Rule<AST::Function()>::type FunctionDefinition;
};

