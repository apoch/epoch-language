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

bool Assignment::Validate(const Program& program) const
{
	// TODO - assignment type validation
	return true;
}

bool Assignment::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	return RHS->TypeInference(program, activescope, context);
}


AssignmentChainExpression::AssignmentChainExpression(Expression* expression)
	: MyExpression(expression)
{
}

AssignmentChainExpression::~AssignmentChainExpression()
{
	delete MyExpression;
}

VM::EpochTypeID AssignmentChainExpression::GetEpochType(const Program& program) const
{
	return MyExpression->GetEpochType(program);
}

bool AssignmentChainExpression::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	return MyExpression->TypeInference(program, activescope, context, 0);
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

VM::EpochTypeID AssignmentChainAssignment::GetEpochType(const Program& program) const
{
	return MyAssignment->GetRHS()->GetEpochType(program);
}

bool AssignmentChainAssignment::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context)
{
	return MyAssignment->GetRHS()->TypeInference(program, activescope, context);
}
