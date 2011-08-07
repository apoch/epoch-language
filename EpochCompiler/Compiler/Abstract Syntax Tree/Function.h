//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for function definitions
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"


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
		DeferredExpressionVector Parameters;
	};

	//
	// Type shortcut for a list of function tags
	//
	typedef std::vector<FunctionTag, Memory::OneWayAlloc<FunctionTag> > FunctionTagList;


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
		Deferred<Expression, boost::intrusive_ptr<Expression> > Return;
		FunctionTagList Tags;
		DeferredCodeBlock Code;
	};

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::Function>,
	(AST::IdentifierT, Content->Name)
	(AST::FunctionParamVec, Content->Parameters)
	(AST::DeferredExpression, Content->Return)
	(AST::FunctionTagList, Content->Tags)
	(AST::DeferredCodeBlock, Content->Code)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::FunctionTag,
	(AST::IdentifierT, TagName)
	(AST::DeferredExpressionVector, Parameters)
)
