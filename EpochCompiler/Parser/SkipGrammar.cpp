#include "pch.h"
#include "Parser/SkipGrammar.h"


SkipGrammar::SkipGrammar()
	: SkipGrammar::base_type(start)
{
	using namespace boost::spirit;

	start = (standard_wide::space)
		  | (L"//" >> *(qi::char_ - qi::eol) >> qi::eol)
		  ;
}
