//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Forward declarations of AST nodes and their deferred forms
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/DeferredNode.h"


namespace AST
{

	struct Expression;
	struct ExpressionComponent;
	struct ExpressionFragment;
	struct PreOperatorStatement;
	struct PostOperatorStatement;
	struct IdentifierListRaw;
	struct ExpressionComponentInternal;
	struct Statement;
	struct FunctionReferenceSignature;
	struct Assignment;
	struct SimpleAssignment;
	struct ChainedEntity;
	struct FunctionParameter;
	struct PostfixEntity;
	struct Entity;
	struct CodeBlock;
	struct ExpressionComponentInternal;
	struct Structure;
	struct Function;
	struct NamedFunctionParameter;

	typedef Deferred<PreOperatorStatement, boost::intrusive_ptr<PreOperatorStatement> > DeferredPreOperatorStatement;
	typedef Deferred<PostOperatorStatement, boost::intrusive_ptr<PostOperatorStatement> > DeferredPostOperatorStatement;
	typedef Deferred<Statement, boost::intrusive_ptr<Statement> > DeferredStatement;
	typedef Deferred<Expression, boost::intrusive_ptr<Expression> > DeferredExpression;
	typedef Deferred<Assignment, boost::intrusive_ptr<Assignment> > DeferredAssignment;
	typedef Deferred<SimpleAssignment, boost::intrusive_ptr<SimpleAssignment> > DeferredSimpleAssignment;
	typedef Deferred<Entity, boost::intrusive_ptr<Entity> > DeferredEntity;
	typedef Deferred<PostfixEntity, boost::intrusive_ptr<PostfixEntity> > DeferredPostfixEntity;
	typedef Deferred<CodeBlock, boost::intrusive_ptr<CodeBlock> > DeferredCodeBlock;
	typedef Deferred<ChainedEntity, boost::intrusive_ptr<ChainedEntity> > DeferredChainedEntity;
	typedef Deferred<ExpressionComponent, boost::intrusive_ptr<ExpressionComponent> > DeferredExpressionComponent;
	typedef Deferred<ExpressionFragment, boost::intrusive_ptr<ExpressionFragment> > DeferredExpressionFragment;
	typedef Deferred<FunctionParameter, boost::intrusive_ptr<FunctionParameter> > DeferredFunctionParameter;
	typedef Deferred<FunctionReferenceSignature, boost::intrusive_ptr<FunctionReferenceSignature> > DeferredFunctionRefSig;
	typedef Deferred<NamedFunctionParameter, boost::intrusive_ptr<NamedFunctionParameter> > DeferredNamedFunctionParameter;
	typedef Deferred<Structure, boost::intrusive_ptr<Structure> > DeferredStructure;

	typedef std::vector<DeferredExpressionFragment, Memory::OneWayAlloc<DeferredExpressionFragment> > DeferredExpressionFragmentVector;
	typedef std::vector<DeferredExpression, Memory::OneWayAlloc<DeferredExpression> > DeferredExpressionVector;
	typedef std::vector<DeferredChainedEntity, Memory::OneWayAlloc<DeferredChainedEntity> > ChainedEntityVector;

}

// Auxiliary definitions
#include "Compiler/Abstract Syntax Tree/NoThrows.h"
