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

	//
	// An algebraic sum type is a discriminated union
	// of two or more other types. Any of the base
	// types can be stored in the union, but only one
	// at a time.
	struct SumType
	{
		IdentifierT SumTypeName;
		IdentifierList BaseTypes;
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

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredSumType,
	(AST::IdentifierT, Content->SumTypeName)
	(AST::IdentifierList, Content->BaseTypes)
)

// TODO - custom allocation for type definition nodes
