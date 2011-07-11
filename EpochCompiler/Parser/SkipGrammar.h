#pragma once

// TODO - documentation improvement


//
// Grammar for defining what to skip during the parse phase.
// We ignore whitespace and code comments.
//
struct SkipGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide>
{
	SkipGrammar();
	boost::spirit::qi::rule<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide> start;
};

