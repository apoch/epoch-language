#pragma once

#include "Parser/SkipGrammar.h"

struct LiteralGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar>
{
	typedef std::wstring::const_iterator IteratorT;

	LiteralGrammar();


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};

	typedef Rule<boost::spirit::qi::unused_type>::type RuleType;

	RuleType StringLiteral;
	RuleType IntegerLiteral;
	RuleType BooleanLiteral;
	RuleType RealLiteral;
	RuleType HexLiteral;
	RuleType Literal;
};

