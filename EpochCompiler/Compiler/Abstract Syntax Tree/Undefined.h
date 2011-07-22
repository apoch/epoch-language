#pragma once


namespace AST
{

	struct Undefined
	{
		Undefined() { }
		Undefined(const boost::spirit::unused_type&) { }
	};

}

