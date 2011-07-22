#pragma once

#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"


namespace AST
{

	struct FunctionTag
	{
		IdentifierT TagName;
		DeferredExpressionVector Parameters;
	};

	typedef std::vector<FunctionTag, Memory::OneWayAlloc<FunctionTag> > FunctionTagList;

	struct Function
	{
		IdentifierT Name;
		FunctionParamVec Parameters;
		Deferred<Expression, boost::intrusive_ptr<Expression> > Return;
		FunctionTagList Tags;
		DeferredCodeBlock Code;
	};

}



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
