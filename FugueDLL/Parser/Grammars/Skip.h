//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Grammar defining stuff we don't want to parse
//

#pragma once

#include <boost/spirit/include/classic.hpp>

#include "Parser/Grammars/Util.h"


namespace Parser
{

	//
	// Grammar for defining what to skip during the parse phase.
	// We ignore whitespace and code comments.
	//
	struct SkipGrammar : public boost::spirit::classic::grammar<SkipGrammar>
	{
		template <typename ScannerType>
		struct definition
		{
			definition(const SkipGrammar&)
			{
				using namespace boost::spirit::classic;

				Skip
					= space_p
					| '\r'
					| '\n'
					| '\t'
					| comment_p(KEYWORD(Comment))
					;
			}

			boost::spirit::classic::rule<ScannerType> Skip;

			//
			// Retrieve the parser's starting (root) symbol
			//
			const boost::spirit::classic::rule<ScannerType>& start() const
			{ return Skip; }
		};
	};


}


#undef KEYWORD
#undef OPERATOR

