#pragma once

#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Forwards.h"

namespace AST
{

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

	typedef boost::variant
		<
			Undefined,
			DeferredNamedFunctionParameter,
			Deferred<struct Expression, boost::intrusive_ptr<Expression> >,
			Deferred<struct FunctionReferenceSignature, boost::intrusive_ptr<FunctionReferenceSignature> >
		> FunctionParameterVariant;

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


	typedef std::vector<AST::DeferredFunctionParameter, Memory::OneWayAlloc<AST::DeferredFunctionParameter> > FunctionParamVec;

}


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

