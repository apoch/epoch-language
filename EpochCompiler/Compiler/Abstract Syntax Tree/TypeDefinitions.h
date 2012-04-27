//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for various type definitions
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"


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

	//
	// A strong alias maps a custom type name onto an
	// existing type's representation, but does not
	// permit other types with the same representation
	// to be interchanged with the newly defined type.
	//
	struct StrongTypeAlias
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

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredStrongTypeAlias,
	(AST::IdentifierT, Content->AliasName)
	(AST::IdentifierT, Content->RepresentationName)
)

// TODO - custom allocation for type definition nodes
