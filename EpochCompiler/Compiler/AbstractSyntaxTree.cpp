#include "pch.h"

#include "Compiler/AbstractSyntaxTree.h"


void AST::intrusive_ptr_add_ref(Expression* expr)
{
	++expr->RefCount;
}

void AST::intrusive_ptr_release(Expression* expr)
{
	if(--expr->RefCount == 0)
		Deallocate(expr);
}


void AST::intrusive_ptr_add_ref(ExpressionComponent* expr)
{
	++expr->RefCount;
}

void AST::intrusive_ptr_release(ExpressionComponent* expr)
{
	if(--expr->RefCount == 0)
		Deallocate(expr);
}


void AST::intrusive_ptr_add_ref(ExpressionFragment* expr)
{
	++expr->RefCount;
}

void AST::intrusive_ptr_release(ExpressionFragment* expr)
{
	if(--expr->RefCount == 0)
		Deallocate(expr);
}


void AST::intrusive_ptr_add_ref(PreOperatorStatement* stmt)
{
	++stmt->RefCount;
}

void AST::intrusive_ptr_release(PreOperatorStatement* stmt)
{
	if(--stmt->RefCount == 0)
		Deallocate(stmt);
}


void AST::intrusive_ptr_add_ref(PostOperatorStatement* stmt)
{
	++stmt->RefCount;
}

void AST::intrusive_ptr_release(PostOperatorStatement* stmt)
{
	if(--stmt->RefCount == 0)
		Deallocate(stmt);
}


void AST::intrusive_ptr_add_ref(IdentifierListRaw* ids)
{
	++ids->RefCount;
}

void AST::intrusive_ptr_release(IdentifierListRaw* ids)
{
	if(--ids->RefCount == 0)
		Deallocate(ids);
}


void AST::intrusive_ptr_add_ref(ExpressionComponentInternal* eci)
{
	++eci->RefCount;
}

void AST::intrusive_ptr_release(ExpressionComponentInternal* eci)
{
	if(--eci->RefCount == 0)
		Deallocate(eci);
}


void AST::intrusive_ptr_add_ref(Statement* stmt)
{
	++stmt->RefCount;
}

void AST::intrusive_ptr_release(Statement* stmt)
{
	if(--stmt->RefCount == 0)
		Deallocate(stmt);
}


void AST::intrusive_ptr_add_ref(FunctionReferenceSignature* sig)
{
	++sig->RefCount;
}

void AST::intrusive_ptr_release(FunctionReferenceSignature* sig)
{
	if(--sig->RefCount == 0)
		Deallocate(sig);
}


void AST::intrusive_ptr_add_ref(Assignment* ast)
{
	++ast->RefCount;
}

void AST::intrusive_ptr_release(Assignment* ast)
{
	if(--ast->RefCount == 0)
		Deallocate(ast);
}


void AST::intrusive_ptr_add_ref(SimpleAssignment* ast)
{
	++ast->RefCount;
}

void AST::intrusive_ptr_release(SimpleAssignment* ast)
{
	if(--ast->RefCount == 0)
		Deallocate(ast);
}


void AST::intrusive_ptr_add_ref(ChainedEntity* entity)
{
	++entity->RefCount;
}

void AST::intrusive_ptr_release(ChainedEntity* entity)
{
	if(--entity->RefCount == 0)
		Deallocate(entity);
}


void AST::intrusive_ptr_add_ref(FunctionParameter* param)
{
	++param->RefCount;
}

void AST::intrusive_ptr_release(FunctionParameter* param)
{
	if(--param->RefCount == 0)
		Deallocate(param);
}


void AST::intrusive_ptr_add_ref(Entity* entity)
{
	++entity->RefCount;
}

void AST::intrusive_ptr_release(Entity* entity)
{
	if(--entity->RefCount == 0)
		Deallocate(entity);
}


void AST::intrusive_ptr_add_ref(PostfixEntity* entity)
{
	++entity->RefCount;
}

void AST::intrusive_ptr_release(PostfixEntity* entity)
{
	if(--entity->RefCount == 0)
		Deallocate(entity);
}


void AST::intrusive_ptr_add_ref(CodeBlock* block)
{
	++block->RefCount;
}

void AST::intrusive_ptr_release(CodeBlock* block)
{
	if(--block->RefCount == 0)
		Deallocate(block);
}
