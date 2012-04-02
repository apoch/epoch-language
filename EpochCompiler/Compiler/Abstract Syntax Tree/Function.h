//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for function definitions
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"


namespace AST
{

	//
	// An individual tag applied to a function definition
	//
	// Function tags provide metadata about the function implementation to
	// the compiler and/or virtual machine. A common tag is "external" which
	// allows functions to defer their implementations to a C-ABI DLL.
	//
	struct FunctionTag
	{
		IdentifierT TagName;
		std::vector<LiteralToken> Parameters;
	};

	//
	// Type shortcut for a list of function tags
	//
	typedef std::vector<FunctionTag, Memory::OneWayAlloc<FunctionTag> > FunctionTagList;

	typedef boost::variant
		<
			Undefined,
			FunctionTagList
		> OptionalFunctionTags;


	//
	// Returns can be either a variable initialization or expression
	//
	typedef boost::variant
		<
			Undefined,
			DeferredInitialization,
			DeferredExpression
		> OptionalReturn;

	//
	// A function definition
	//
	// This includes the function's name, its parameter set,
	// its return value expression, any tags, and its code.
	//
	struct Function
	{
		IdentifierT Name;
		FunctionParamVec Parameters;
		OptionalReturn Return;
		OptionalFunctionTags Tags;
		OptionalCodeBlock Code;

		Function()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		Function(const Function&);
		Function& operator = (const Function&);
	};

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredFunction,
	(AST::IdentifierT, Content->Name)
	(AST::FunctionParamVec, Content->Parameters)
	(AST::OptionalReturn, Content->Return)
	(AST::OptionalFunctionTags, Content->Tags)
	(AST::OptionalCodeBlock, Content->Code)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionTag,
	(AST::IdentifierT, TagName)
	(std::vector<AST::LiteralToken>, Parameters)
)
