//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for various type definitions
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/IdentifierT.h"


namespace AST
{

	//
	// A simple alias maps a custom type name onto an
	// existing type's representation.
	//
	struct TypeAlias
	{
		IdentifierT AliasName;
		IdentifierT RepresentationName;
	};

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredTypeAlias,
	(AST::IdentifierT, Content->AliasName)
	(AST::IdentifierT, Content->RepresentationName)
)

// TODO - custom allocation for type definition nodes
