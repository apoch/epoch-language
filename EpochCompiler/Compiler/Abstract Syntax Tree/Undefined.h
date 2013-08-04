//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Dummy/placeholder AST node for an omitted or unrecognized leaf
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
	// More commonly, it is also used for holding omitted
	// branches of the AST, i.e. as a marker indicating
	// that no child nodes will be provided. This is used
	// for handling optional syntactical elements.
	//
	struct Undefined
	{
		Undefined() { }
		Undefined(const boost::spirit::unused_type&) { }
	};

}

