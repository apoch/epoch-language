#pragma once

#include "Parser/SkipGrammar.h"

#include "Compiler/AbstractSyntaxTree.h"


struct LiteralGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::LiteralToken()>
{
	typedef std::wstring::const_iterator IteratorT;

	LiteralGrammar();


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};

	Rule<AST::LiteralStringT()>::type StringLiteral;
	Rule<Real32()>::type RealLiteral;
	Rule<UInteger32()>::type HexLiteral;
	Rule<AST::LiteralToken()>::type Literal;

	boost::spirit::qi::int_parser<Integer32> Integer32Parser; 
	boost::spirit::qi::real_parser<Real32, boost::spirit::qi::strict_real_policies<Real32> > Real32Parser;
	boost::spirit::qi::symbols<wchar_t, bool> BooleanLiteral;
};

