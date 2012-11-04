//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST type definitions for generic programming features
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"


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

}


BOOST_FUSION_ADAPT_STRUCT
(
	AST::TemplateParameter,
	(AST::IdentifierT, Type)
	(AST::IdentifierT, Name)
)
