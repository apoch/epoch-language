#pragma once

typedef std::wstring::const_iterator positertype;

namespace AST
{

	//
	// An identifier is simply a pair of iterators pointing
	// into the original code text buffer, thus eliminating
	// the overhead of making a string copy of every single
	// identifier in the program.
	//
	typedef boost::iterator_range<positertype> IdentifierT;

}

