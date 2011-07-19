#pragma once


#include <iostream>


struct GrammarDebugger
{
	template <typename Iterator, typename Context, typename State>
	void operator() (const Iterator& first, const Iterator& last, const Context& context, State state, const std::string& rule_name) const
	{
		std::wstring widename(rule_name.begin(), rule_name.end());
		if(state == boost::spirit::qi::pre_parse)
		{
			std::wcout << L"Trying rule " << widename << L" on input:\n";
			for(Iterator iter = first; iter != last; ++iter)
			{
				std::wcout << L"Token ID " << iter->id() << L" ";
				boost::spirit::traits::print_token(std::wcout, *iter);
				std::wcout << L"\n";
			}
			std::wcout << L"\nEND OF ATTEMPT\n" << std::endl;
		}
		else if(state == boost::spirit::qi::successful_parse)
		{
			std::wcout << L"SUCCESS" << std::endl;
		}
		else
		{
			std::wcout << L"FAIL" << std::endl;
		}
	}
};

