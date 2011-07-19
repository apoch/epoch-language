#include "pch.h"

#include "Compiler/AbstractSyntaxTree.h"


void AST::intrusive_ptr_add_ref(Expression* expr)
{
	::InterlockedIncrement(&expr->RefCount);
}

void AST::intrusive_ptr_release(Expression* expr)
{
	if(::InterlockedDecrement(&expr->RefCount) == 0)
		delete expr;
}


void AST::intrusive_ptr_add_ref(ExpressionComponent* expr)
{
	::InterlockedIncrement(&expr->RefCount);
}

void AST::intrusive_ptr_release(ExpressionComponent* expr)
{
	if(::InterlockedDecrement(&expr->RefCount) == 0)
		delete expr;
}


void AST::intrusive_ptr_add_ref(ExpressionFragment* expr)
{
	::InterlockedIncrement(&expr->RefCount);
}

void AST::intrusive_ptr_release(ExpressionFragment* expr)
{
	if(::InterlockedDecrement(&expr->RefCount) == 0)
		delete expr;
}


void AST::intrusive_ptr_add_ref(PreOperatorStatement* stmt)
{
	::InterlockedIncrement(&stmt->RefCount);
}

void AST::intrusive_ptr_release(PreOperatorStatement* stmt)
{
	if(::InterlockedDecrement(&stmt->RefCount) == 0)
		delete stmt;
}


void AST::intrusive_ptr_add_ref(PostOperatorStatement* stmt)
{
	::InterlockedIncrement(&stmt->RefCount);
}

void AST::intrusive_ptr_release(PostOperatorStatement* stmt)
{
	if(::InterlockedDecrement(&stmt->RefCount) == 0)
		delete stmt;
}


void AST::intrusive_ptr_add_ref(IdentifierListRaw* ids)
{
	::InterlockedIncrement(&ids->RefCount);
}

void AST::intrusive_ptr_release(IdentifierListRaw* ids)
{
	if(::InterlockedDecrement(&ids->RefCount) == 0)
		delete ids;
}


void AST::intrusive_ptr_add_ref(ExpressionComponentInternal* eci)
{
	::InterlockedIncrement(&eci->RefCount);
}

void AST::intrusive_ptr_release(ExpressionComponentInternal* eci)
{
	if(::InterlockedDecrement(&eci->RefCount) == 0)
		delete eci;
}


void AST::intrusive_ptr_add_ref(Statement* stmt)
{
	::InterlockedIncrement(&stmt->RefCount);
}

void AST::intrusive_ptr_release(Statement* stmt)
{
	if(::InterlockedDecrement(&stmt->RefCount) == 0)
		delete stmt;
}
