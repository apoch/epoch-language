//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Reference counting implementation for AST nodes
//
// Note that we prefer a series of free functions per AST node type
// over a shared base class, because this approach avoids the need
// for expensive virtual destructors.
//

#include "pch.h"

#include "Compiler/Abstract Syntax Tree/RefCounting.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"
#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Function.h"


namespace
{
	//
	// Helper for adding to a reference count
	//
	template<typename T>
	void AddRef(T* p)
	{
		++p->RefCount;
	}

	//
	// Helper for releasing a referenced object
	//
	template<typename T>
	void ReleaseRef(T* p)
	{
		if(--p->RefCount == 0)
			Deallocate(p);
	}
}


void AST::intrusive_ptr_add_ref(IdentifierListRaw* ids)				{ AddRef(ids); }
void AST::intrusive_ptr_release(IdentifierListRaw* ids)				{ ReleaseRef(ids); }

void AST::intrusive_ptr_add_ref(Expression* expr)					{ AddRef(expr); }
void AST::intrusive_ptr_release(Expression* expr)					{ ReleaseRef(expr); }

void AST::intrusive_ptr_add_ref(ExpressionComponent* expr)			{ AddRef(expr); }
void AST::intrusive_ptr_release(ExpressionComponent* expr)			{ ReleaseRef(expr); }

void AST::intrusive_ptr_add_ref(ExpressionComponentInternal* eci)	{ AddRef(eci); }
void AST::intrusive_ptr_release(ExpressionComponentInternal* eci)	{ ReleaseRef(eci); }

void AST::intrusive_ptr_add_ref(ExpressionFragment* expr)			{ AddRef(expr); }
void AST::intrusive_ptr_release(ExpressionFragment* expr)			{ ReleaseRef(expr); }

void AST::intrusive_ptr_add_ref(PreOperatorStatement* stmt)			{ AddRef(stmt); }
void AST::intrusive_ptr_release(PreOperatorStatement* stmt)			{ ReleaseRef(stmt); }

void AST::intrusive_ptr_add_ref(PostOperatorStatement* stmt)		{ AddRef(stmt); }
void AST::intrusive_ptr_release(PostOperatorStatement* stmt)		{ ReleaseRef(stmt); }

void AST::intrusive_ptr_add_ref(Statement* stmt)					{ AddRef(stmt); }
void AST::intrusive_ptr_release(Statement* stmt)					{ ReleaseRef(stmt); }

void AST::intrusive_ptr_add_ref(Assignment* ast)					{ AddRef(ast); }
void AST::intrusive_ptr_release(Assignment* ast)					{ ReleaseRef(ast); }

void AST::intrusive_ptr_add_ref(SimpleAssignment* ast)				{ AddRef(ast); }
void AST::intrusive_ptr_release(SimpleAssignment* ast)				{ ReleaseRef(ast); }

void AST::intrusive_ptr_add_ref(Initialization* init)				{ AddRef(init); }
void AST::intrusive_ptr_release(Initialization* init)				{ ReleaseRef(init); }

void AST::intrusive_ptr_add_ref(ChainedEntity* entity)				{ AddRef(entity); }
void AST::intrusive_ptr_release(ChainedEntity* entity)				{ ReleaseRef(entity); }

void AST::intrusive_ptr_add_ref(Entity* entity)						{ AddRef(entity); }
void AST::intrusive_ptr_release(Entity* entity)						{ ReleaseRef(entity); }

void AST::intrusive_ptr_add_ref(PostfixEntity* entity)				{ AddRef(entity); }
void AST::intrusive_ptr_release(PostfixEntity* entity)				{ ReleaseRef(entity); }

void AST::intrusive_ptr_add_ref(CodeBlock* block)					{ AddRef(block); }
void AST::intrusive_ptr_release(CodeBlock* block)					{ ReleaseRef(block); }

void AST::intrusive_ptr_add_ref(Function* fn)						{ AddRef(fn); }
void AST::intrusive_ptr_release(Function* fn)						{ ReleaseRef(fn); }

void AST::intrusive_ptr_add_ref(FunctionParameter* param)			{ AddRef(param); }
void AST::intrusive_ptr_release(FunctionParameter* param)			{ ReleaseRef(param); }

void AST::intrusive_ptr_add_ref(NamedFunctionParameter* nfp)		{ AddRef(nfp); }
void AST::intrusive_ptr_release(NamedFunctionParameter* nfp)		{ ReleaseRef(nfp); }

void AST::intrusive_ptr_add_ref(Structure* st)						{ AddRef(st); }
void AST::intrusive_ptr_release(Structure* st)						{ ReleaseRef(st); }

void AST::intrusive_ptr_add_ref(FunctionReferenceSignature* sig)	{ AddRef(sig); }
void AST::intrusive_ptr_release(FunctionReferenceSignature* sig)	{ ReleaseRef(sig); }

