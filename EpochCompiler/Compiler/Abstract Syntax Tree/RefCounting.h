//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Reference counting and allocation declarations for AST nodes
//

#pragma once


// Dependencies
#include "Utility/Memory/OneWayAllocator.h"
#include "Compiler/Abstract Syntax Tree/Forwards.h"


namespace AST
{
	template <typename T>
	inline T* Allocate()
	{
		return new T;
	}

	template <typename T>
	inline void Deallocate(T* ptr)
	{
		delete ptr;
	}

	template<> inline Expression* Allocate<Expression>()									{ return Memory::OneWayAllocateObject<Expression>(1); }
	template<> inline ExpressionComponent* Allocate<ExpressionComponent>()					{ return Memory::OneWayAllocateObject<ExpressionComponent>(1); }
	template<> inline ExpressionFragment* Allocate<ExpressionFragment>()					{ return Memory::OneWayAllocateObject<ExpressionFragment>(1); }
	template<> inline PreOperatorStatement* Allocate<PreOperatorStatement>()				{ return Memory::OneWayAllocateObject<PreOperatorStatement>(1); }
	template<> inline PostOperatorStatement* Allocate<PostOperatorStatement>()				{ return Memory::OneWayAllocateObject<PostOperatorStatement>(1); }
	template<> inline IdentifierListRaw* Allocate<IdentifierListRaw>()						{ return Memory::OneWayAllocateObject<IdentifierListRaw>(1); }
	template<> inline ExpressionComponentInternal* Allocate<ExpressionComponentInternal>()	{ return Memory::OneWayAllocateObject<ExpressionComponentInternal>(1); }
	template<> inline Statement* Allocate<Statement>()										{ return Memory::OneWayAllocateObject<Statement>(1); }
	template<> inline FunctionReferenceSignature* Allocate<FunctionReferenceSignature>()	{ return Memory::OneWayAllocateObject<FunctionReferenceSignature>(1); }
	template<> inline Assignment* Allocate<Assignment>()									{ return Memory::OneWayAllocateObject<Assignment>(1); }
	template<> inline SimpleAssignment* Allocate<SimpleAssignment>()						{ return Memory::OneWayAllocateObject<SimpleAssignment>(1); }
	template<> inline ChainedEntity* Allocate<ChainedEntity>()								{ return Memory::OneWayAllocateObject<ChainedEntity>(1); }
	template<> inline FunctionParameter* Allocate<FunctionParameter>()						{ return Memory::OneWayAllocateObject<FunctionParameter>(1); }
	template<> inline Entity* Allocate<Entity>()											{ return Memory::OneWayAllocateObject<Entity>(1); }
	template<> inline PostfixEntity* Allocate<PostfixEntity>()								{ return Memory::OneWayAllocateObject<PostfixEntity>(1); }
	template<> inline CodeBlock* Allocate<CodeBlock>()										{ return Memory::OneWayAllocateObject<CodeBlock>(1); }
	template<> inline NamedFunctionParameter* Allocate<NamedFunctionParameter>()			{ return Memory::OneWayAllocateObject<NamedFunctionParameter>(1); }
	template<> inline Structure* Allocate<Structure>()										{ return Memory::OneWayAllocateObject<Structure>(1); }
	template<> inline Function* Allocate<Function>()										{ return Memory::OneWayAllocateObject<Function>(1); }

	template<> inline void Deallocate(Expression* p)					{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(ExpressionComponent* p)			{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(ExpressionFragment* p)			{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(PreOperatorStatement* p)			{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(PostOperatorStatement* p)			{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(IdentifierListRaw* p)				{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(ExpressionComponentInternal* p)	{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(Statement* p)						{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(FunctionReferenceSignature* p)	{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(Assignment* p)					{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(SimpleAssignment* p)				{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(ChainedEntity* p)					{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(FunctionParameter* p)				{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(Entity* p)						{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(PostfixEntity* p)					{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(CodeBlock* p)						{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(NamedFunctionParameter* p)		{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(Structure* p)						{ Memory::OneWayRecordDeallocObject(p); }
	template<> inline void Deallocate(Function* p)						{ Memory::OneWayRecordDeallocObject(p); }

	void intrusive_ptr_add_ref(Expression* expr);
	void intrusive_ptr_release(Expression* expr);

	void intrusive_ptr_add_ref(ExpressionComponent* expr);
	void intrusive_ptr_release(ExpressionComponent* expr);

	void intrusive_ptr_add_ref(ExpressionFragment* expr);
	void intrusive_ptr_release(ExpressionFragment* expr);

	void intrusive_ptr_add_ref(PreOperatorStatement* expr);
	void intrusive_ptr_release(PreOperatorStatement* expr);

	void intrusive_ptr_add_ref(PostOperatorStatement* expr);
	void intrusive_ptr_release(PostOperatorStatement* expr);

	void intrusive_ptr_add_ref(IdentifierListRaw* expr);
	void intrusive_ptr_release(IdentifierListRaw* expr);

	void intrusive_ptr_add_ref(ExpressionComponentInternal* expr);
	void intrusive_ptr_release(ExpressionComponentInternal* expr);

	void intrusive_ptr_add_ref(Statement* expr);
	void intrusive_ptr_release(Statement* expr);

	void intrusive_ptr_add_ref(FunctionReferenceSignature* expr);
	void intrusive_ptr_release(FunctionReferenceSignature* expr);

	void intrusive_ptr_add_ref(Assignment* expr);
	void intrusive_ptr_release(Assignment* expr);

	void intrusive_ptr_add_ref(SimpleAssignment* expr);
	void intrusive_ptr_release(SimpleAssignment* expr);

	void intrusive_ptr_add_ref(Entity* entity);
	void intrusive_ptr_release(Entity* entity);

	void intrusive_ptr_add_ref(ChainedEntity* entity);
	void intrusive_ptr_release(ChainedEntity* entity);

	void intrusive_ptr_add_ref(FunctionParameter* param);
	void intrusive_ptr_release(FunctionParameter* param);

	void intrusive_ptr_add_ref(PostfixEntity* entity);
	void intrusive_ptr_release(PostfixEntity* entity);

	void intrusive_ptr_add_ref(CodeBlock* block);
	void intrusive_ptr_release(CodeBlock* block);

	void intrusive_ptr_add_ref(NamedFunctionParameter* nfp);
	void intrusive_ptr_release(NamedFunctionParameter* nfp);

	void intrusive_ptr_add_ref(Structure* st);
	void intrusive_ptr_release(Structure* st);

	void intrusive_ptr_add_ref(Function* st);
	void intrusive_ptr_release(Function* st);

}

