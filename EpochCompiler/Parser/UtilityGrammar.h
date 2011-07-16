#pragma once

#include "Parser/SkipGrammar.h"
#include "Compiler/AbstractSyntaxTree.h"

struct UtilityGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::IdentifierT()>
{
	typedef std::wstring::const_iterator IteratorT;

	UtilityGrammar();


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};


	Rule<AST::IdentifierT()>::type StringIdentifier;
};

