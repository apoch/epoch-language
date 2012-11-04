//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST type definitions for generic programming features
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Literals.h"


namespace AST
{

	struct TemplateParameter
	{
		IdentifierT Type;
		IdentifierT Name;
	};


	typedef std::vector<TemplateParameter> TemplateParameterList;

	typedef boost::variant
		<
			Undefined,
			TemplateParameterList
		> OptionalTemplateParameterList;


	typedef boost::variant
		<
			Undefined,
			IdentifierT,
			LiteralToken
		> TemplateArgument;


	typedef std::vector<TemplateArgument> TemplateArgumentList;


	typedef boost::variant
		<
			Undefined,
			TemplateArgumentList
		> OptionalTemplateArgumentList;

}


BOOST_FUSION_ADAPT_STRUCT
(
	AST::TemplateParameter,
	(AST::IdentifierT, Type)
	(AST::IdentifierT, Name)
)
