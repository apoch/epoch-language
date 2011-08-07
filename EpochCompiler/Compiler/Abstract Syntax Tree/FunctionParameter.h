//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for function parameter forms
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Forwards.h"


namespace AST
{

	//
	// AST node describing a named function parameter
	//
	// Named parameters consist of a type and a name, and bind directly
	// to local variables within the function's lexical scope.
	//
	struct NamedFunctionParameter
	{
		IdentifierT Type;
		IdentifierT Name;

		long RefCount;

		NamedFunctionParameter()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		NamedFunctionParameter(const NamedFunctionParameter&);
		NamedFunctionParameter& operator = (const NamedFunctionParameter&);
	};

	//
	// Variant describing any valid function parameter form
	//
	// Forms accepted include:
	//		- Named function parameters (see above)
	//		- Expressions (for pattern matching)
	//		- Function reference signatures (for higher order functions)
	//
	typedef boost::variant
		<
			Undefined,
			DeferredNamedFunctionParameter,
			DeferredExpression,
			DeferredFunctionRefSig
		> FunctionParameterVariant;

	//
	// Wrapper for holding a function parameter variant
	// and reference counting it efficiently.
	//
	struct FunctionParameter
	{
		FunctionParameterVariant V;

		long RefCount;

		FunctionParameter()
			: RefCount(0)
		{ }

		FunctionParameter(const FunctionParameterVariant& v)
			: V(v),
			  RefCount(0)
		{ }

	// Non-copyable
	private:
		FunctionParameter(const FunctionParameter&);
		FunctionParameter& operator = (const FunctionParameter&);
	};

	//
	// Function reference signature
	//
	// A function reference signature defines the types of functions which are
	// allowed to be passed as arguments to a higher-order function. For example,
	// a reference signature might require that any passed function accepts two
	// integers and returns a boolean.
	//
	struct FunctionReferenceSignature
	{
		IdentifierT Identifier;
		IdentifierList ParamTypes;
		IdentifierT ReturnType;

		FunctionReferenceSignature()
			: RefCount(0)
		{ }

		long RefCount;

	// Non-copyable
	private:
		FunctionReferenceSignature(const FunctionReferenceSignature&);
		FunctionReferenceSignature& operator = (const FunctionReferenceSignature&);
	};


	//
	// Type shortcut for a list of function parameters
	//
	typedef std::vector<AST::DeferredFunctionParameter, Memory::OneWayAlloc<AST::DeferredFunctionParameter> > FunctionParamVec;

}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredNamedFunctionParameter,
	(AST::IdentifierT, Content->Type)
	(AST::IdentifierT, Content->Name)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredFunctionRefSig,
	(AST::IdentifierT, Content->Identifier)
	(AST::IdentifierList, Content->ParamTypes)
	(AST::IdentifierT, Content->ReturnType)
)

