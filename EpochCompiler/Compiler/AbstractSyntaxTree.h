//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Node classes for constructing abstract syntax trees for Epoch programs
//

#pragma once


// Dependencies
#include <boost/fusion/adapted.hpp>
#include <string>
#include <vector>


namespace AST
{

	struct FunctionParameter
	{
		std::wstring Type;
		std::wstring Name;
	};

	struct Function
	{
		std::wstring Name;
		std::vector<FunctionParameter> Parameters;
	};

	struct Program
	{
		Program() { }
		Program(const std::vector<Function>& functions);
	};

}

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Function,
	(std::wstring, Name)
	(std::vector<AST::FunctionParameter>, Parameters)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionParameter,
	(std::wstring, Type)
	(std::wstring, Name)
)

