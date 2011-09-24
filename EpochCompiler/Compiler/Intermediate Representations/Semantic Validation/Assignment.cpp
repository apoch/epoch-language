//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing assignments
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"


using namespace IRSemantics;



Assignment::Assignment(const std::vector<StringHandle>& lhs, StringHandle operatorname)
	: RHS(NULL),
	  LHS(lhs),
	  OperatorName(operatorname)
{
}

Assignment::~Assignment()
{
	delete RHS;
}

void Assignment::SetRHS(AssignmentChain* rhs)
{
	delete RHS;
	RHS = rhs;
}

void Assignment::SetRHSRecursive(AssignmentChain* rhs)
{
	if(RHS)
	{
		if(!RHS->CanChainToAssignment())
			throw std::exception("Invalid parse state");			// TODO - better exceptions

		RHS->SetRHSRecursive(rhs);
	}
	else
	{
		RHS = rhs;
	}
}


AssignmentChainExpression::AssignmentChainExpression(Expression* expression)
	: MyExpression(expression)
{
}

AssignmentChainExpression::~AssignmentChainExpression()
{
	delete MyExpression;
}



AssignmentChainAssignment::AssignmentChainAssignment(Assignment* assignment)
	: MyAssignment(assignment)
{
}

AssignmentChainAssignment::~AssignmentChainAssignment()
{
	delete MyAssignment;
}

void AssignmentChainAssignment::SetRHSRecursive(AssignmentChain* rhs)
{
	MyAssignment->SetRHSRecursive(rhs);
}

