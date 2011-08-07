//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Dummy/placeholder AST node for an unrecognized leaf
//

#pragma once


namespace AST
{

	//
	// This AST node type is used as a fallback in case
	// of parse failure, when a proper node cannot be
	// constructed. It must be convertible from the type
	// boost::spirit::unused_type to support absorbing
	// synthesized attributes which have failed.
	//
	struct Undefined
	{
		Undefined() { }
		Undefined(const boost::spirit::unused_type&) { }
	};

}

