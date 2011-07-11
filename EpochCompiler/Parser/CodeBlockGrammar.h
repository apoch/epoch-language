#pragma once

#include "Parser/SkipGrammar.h"

struct CodeBlockGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar>
{
	typedef std::wstring::const_iterator IteratorT;

	CodeBlockGrammar ();


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};


	Rule<boost::spirit::qi::unused_type>::type CodeBlock;
};

