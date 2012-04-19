//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing assignments
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/Session.h"

#include "Compiler/Exceptions.h"


using namespace IRSemantics;



Assignment::Assignment(const std::vector<StringHandle>& lhs, StringHandle operatorname)
	: LHS(lhs),
	  OperatorName(operatorname),
	  RHS(NULL),
	  LHSType(VM::EpochType_Error)
{
}

Assignment::~Assignment()
{
	delete RHS;
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
		delete RHS;
		RHS = rhs;
	}
}

bool Assignment::Validate(const Program& program) const
{
	bool valid = (LHSType == RHS->GetEpochType(program)) && (LHSType != VM::EpochType_Error);
	return valid && RHS->Validate(program);
}

bool Assignment::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	LHSType = InferMemberAccessType(LHS, program, activescope);
	if(!RHS->TypeInference(program, activescope, context, errors))
		return false;

	OverloadMap::const_iterator iter = program.Session.FunctionOverloadNames.find(OperatorName);
	if(iter != program.Session.FunctionOverloadNames.end())
	{
		const StringHandleSet& overloads = iter->second;
		for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
		{
			const FunctionSignature& overloadsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
			if(overloadsig.GetNumParameters() == 2)
			{
				if(overloadsig.GetParameter(0).Type == LHSType && overloadsig.GetParameter(1).Type == RHS->GetEpochType(program))
				{
					OperatorName = *overloaditer;
					break;
				}
			}
		}
	}

	return true;
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

bool AssignmentChainExpression::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	InferenceContext newcontext(0, InferenceContext::CONTEXT_ASSIGNMENT);
	newcontext.FunctionName = context.FunctionName;
	return MyExpression->TypeInference(program, activescope, newcontext, 0, 1, errors);
}

bool AssignmentChainExpression::Validate(const Program& program) const
{
	return MyExpression->Validate(program);
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

bool AssignmentChainAssignment::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	return MyAssignment->TypeInference(program, activescope, context, errors);
}

bool AssignmentChainAssignment::Validate(const Program& program) const
{
	return MyAssignment->Validate(program);
}

