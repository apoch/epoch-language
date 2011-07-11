#pragma once

#include "Parser/SkipGrammar.h"
#include "Compiler/AbstractSyntaxTree.h"

struct FunctionDefinitionGrammar;

struct GlobalGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, std::vector<AST::Function>()>
{
	typedef std::wstring::const_iterator IteratorT;

	explicit GlobalGrammar(const FunctionDefinitionGrammar& funcdefgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};

	typedef Rule<boost::spirit::qi::unused_type>::type RuleType;


	Rule<std::vector<AST::Function>()>::type Program;

	RuleType StructureDefinition;
	RuleType GlobalDefinition;
	Rule<AST::Function()>::type MetaEntity;

	const FunctionDefinitionGrammar& TheFunctionDefinitionGrammar;
};