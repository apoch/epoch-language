//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing assignments
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"

#include "Compiler/Exceptions.h"


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
		{
			//
			// This should basically never happen unless the parser is
			// generating gibberish in the AST.
			//
			// Basically this is designed to ensure that everything in
			// the chain is an l-value, and the far right hand side is
			// an r-value. The semantics of this may need to change if
			// we wish to permit arbitrary expressions to be l-values.
			//
			// For clarity, assignments must be of the form:
			//  l-value [ = l-value ...] = r-value
			//
			// Where an l-value is either a primitive variable or a
			// (possibly deeply nested) structure member, and an r-value
			// is an arbitrary expression of compatible type to the
			// chain of l-values.
			//
			throw InternalException("Far-right of assignment or assignment chain cannot participate in further chaining");
		}

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
