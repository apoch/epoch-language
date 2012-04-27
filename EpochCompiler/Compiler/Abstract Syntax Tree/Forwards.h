//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Forward declarations of AST nodes and their deferred forms
//

#pragma once

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
	struct Initialization;
	struct ChainedEntity;
	struct FunctionParameter;
	struct PostfixEntity;
	struct Entity;
	struct CodeBlock;
	struct ExpressionComponentInternal;
	struct Structure;
	struct Function;
	struct NamedFunctionParameter;
	struct TypeAlias;
	struct StrongTypeAlias;

	template <typename T>
	inline T* Allocate();

	template <typename T>
	inline void Deallocate(T* ptr);

	template<> inline Expression* Allocate<Expression>();
	template<> inline ExpressionComponent* Allocate<ExpressionComponent>();
	template<> inline ExpressionFragment* Allocate<ExpressionFragment>();
	template<> inline PreOperatorStatement* Allocate<PreOperatorStatement>();
	template<> inline PostOperatorStatement* Allocate<PostOperatorStatement>();
	template<> inline IdentifierListRaw* Allocate<IdentifierListRaw>();
	template<> inline ExpressionComponentInternal* Allocate<ExpressionComponentInternal>();
	template<> inline Statement* Allocate<Statement>();
	template<> inline FunctionReferenceSignature* Allocate<FunctionReferenceSignature>();
	template<> inline Assignment* Allocate<Assignment>();
	template<> inline SimpleAssignment* Allocate<SimpleAssignment>();
	template<> inline Initialization* Allocate<Initialization>();
	template<> inline ChainedEntity* Allocate<ChainedEntity>();
	template<> inline FunctionParameter* Allocate<FunctionParameter>();
	template<> inline Entity* Allocate<Entity>();
	template<> inline PostfixEntity* Allocate<PostfixEntity>();
	template<> inline CodeBlock* Allocate<CodeBlock>();
	template<> inline NamedFunctionParameter* Allocate<NamedFunctionParameter>();
	template<> inline Structure* Allocate<Structure>();
	template<> inline Function* Allocate<Function>();

	template<> inline void Deallocate(Expression* p);
	template<> inline void Deallocate(ExpressionComponent* p);
	template<> inline void Deallocate(ExpressionFragment* p);
	template<> inline void Deallocate(PreOperatorStatement* p);
	template<> inline void Deallocate(PostOperatorStatement* p);
	template<> inline void Deallocate(IdentifierListRaw* p);
	template<> inline void Deallocate(ExpressionComponentInternal* p);
	template<> inline void Deallocate(Statement* p);
	template<> inline void Deallocate(FunctionReferenceSignature* p);
	template<> inline void Deallocate(Assignment* p);
	template<> inline void Deallocate(SimpleAssignment* p);
	template<> inline void Deallocate(Initialization* p);
	template<> inline void Deallocate(ChainedEntity* p);
	template<> inline void Deallocate(FunctionParameter* p);
	template<> inline void Deallocate(Entity* p);
	template<> inline void Deallocate(PostfixEntity* p);
	template<> inline void Deallocate(CodeBlock* p);
	template<> inline void Deallocate(NamedFunctionParameter* p);
	template<> inline void Deallocate(Structure* p);
	template<> inline void Deallocate(Function* p);
}

// Dependencies
#include "Compiler/Abstract Syntax Tree/DeferredNode.h"


namespace AST
{
	typedef Deferred<PreOperatorStatement, boost::intrusive_ptr<PreOperatorStatement> > DeferredPreOperatorStatement;
	typedef Deferred<PostOperatorStatement, boost::intrusive_ptr<PostOperatorStatement> > DeferredPostOperatorStatement;
	typedef Deferred<Statement, boost::intrusive_ptr<Statement> > DeferredStatement;
	typedef Deferred<Expression, boost::intrusive_ptr<Expression> > DeferredExpression;
	typedef Deferred<Assignment, boost::intrusive_ptr<Assignment> > DeferredAssignment;
	typedef Deferred<SimpleAssignment, boost::intrusive_ptr<SimpleAssignment> > DeferredSimpleAssignment;
	typedef Deferred<Initialization, boost::intrusive_ptr<Initialization> > DeferredInitialization;
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
	typedef Deferred<Function, boost::intrusive_ptr<Function> > DeferredFunction;
	
	typedef Deferred<TypeAlias, boost::shared_ptr<TypeAlias> > DeferredTypeAlias;
	typedef Deferred<StrongTypeAlias, boost::shared_ptr<StrongTypeAlias> > DeferredStrongTypeAlias;

	typedef std::vector<DeferredExpressionFragment, Memory::OneWayAlloc<DeferredExpressionFragment> > DeferredExpressionFragmentVector;
	typedef std::vector<DeferredExpression, Memory::OneWayAlloc<DeferredExpression> > DeferredExpressionVector;
	typedef std::vector<DeferredChainedEntity, Memory::OneWayAlloc<DeferredChainedEntity> > ChainedEntityVector;
}

// Auxiliary definitions
#include "Compiler/Abstract Syntax Tree/NoThrows.h"
