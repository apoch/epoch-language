//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST type definition for identifiers
//
// This is split into a separate, minimal header so that external
// code can refer to AST identifiers for various purposes, e.g.
// error tracking and messaging, without depending on the full AST
// header set.
//

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

