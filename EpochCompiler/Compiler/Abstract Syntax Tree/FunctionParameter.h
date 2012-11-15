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
#include "Compiler/Abstract Syntax Tree/OptionalIdentifier.h"
#include "Compiler/Abstract Syntax Tree/Templates.h"


namespace AST
{

	//
	// Placeholder for flagging parameters as having reference semantics
	//
	struct RefTag
	{
		IdentifierT Ignored;
	};

	typedef boost::variant
		<
			Undefined,
			RefTag
		> OptionalRef;

	//
	// AST node describing a named function parameter
	//
	// Named parameters consist of a type and a name, and bind directly
	// to local variables within the function's lexical scope.
	//
	struct NamedFunctionParameter
	{
		IdentifierT Type;
		OptionalTemplateArgumentList TemplateArgs;
		OptionalRef IsReference;
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
	// Special marker for the "nothing" type dummy
	//
	struct Nothing
	{
		IdentifierT ShouldBeNothing;
	};

	//
	// Variant describing any valid function parameter form
	//
	// Forms accepted include:
	//		- Named function parameters (see above)
	//		- Expressions (for pattern matching)
	//		- Function reference signatures (for higher order functions)
	//		- The "nothing" dummy type
	//
	typedef boost::variant
		<
			Undefined,
			DeferredNamedFunctionParameter,
			DeferredExpression,
			DeferredFunctionRefSig,
			Nothing
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
		OptionalIdentifier ReturnType;

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
	AST::RefTag,
	(AST::IdentifierT, Ignored)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredNamedFunctionParameter,
	(AST::IdentifierT, Content->Type)
	(AST::OptionalTemplateArgumentList, Content->TemplateArgs)
	(AST::OptionalRef, Content->IsReference)
	(AST::IdentifierT, Content->Name)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredFunctionRefSig,
	(AST::IdentifierT, Content->Identifier)
	(AST::IdentifierList, Content->ParamTypes)
	(AST::OptionalIdentifier, Content->ReturnType)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Nothing,
	(AST::IdentifierT, ShouldBeNothing)
)
