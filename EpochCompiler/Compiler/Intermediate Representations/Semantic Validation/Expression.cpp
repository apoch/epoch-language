//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing expressions
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "User Interface/Output.h"


using namespace IRSemantics;


Expression::Expression()
	: InferredType(VM::EpochType_Infer)
{
}


Expression::~Expression()
{
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
		delete *iter;
}


bool Expression::Validate(const Program& program) const
{
	VM::EpochTypeID mytype = GetEpochType(program);
	switch(mytype)
	{
	case VM::EpochType_Error:
		{
			UI::OutputStream output;
			output << L"Expression contains a type error" << std::endl;
		}
		return false;

	case VM::EpochType_Infer:
		{
			UI::OutputStream output;
			output << L"Type inference failed or otherwise incomplete" << std::endl;
		}
		return false;
	}

	return true;
}


bool Expression::CompileTimeCodeExecution(Program& program, CodeBlock& activescope)
{
	Coalesce(program, activescope);

	// TODO - compile time code execution for expressions
	return true;
}

bool Expression::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	bool result = true;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, context, index))
			result = false;
	}

	return result;
}

VM::EpochTypeID Expression::GetEpochType(const Program& program) const
{
	return InferredType;
}

void Expression::AddAtom(ExpressionAtom* atom)
{
	Atoms.push_back(atom);
}

void Expression::Coalesce(Program& program, CodeBlock& activescope)
{
	if(Atoms.empty())
	{
		InferredType = VM::EpochType_Error;
		return;
	}

	bool completed;
	do
	{
		completed = true;
		for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
		{
			ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
			if(!opatom)
				continue;

			if(program.GetString(opatom->GetIdentifier()) == L".")
			{
				std::vector<ExpressionAtom*>::iterator previter = iter;
				--previter;

				std::vector<ExpressionAtom*>::iterator nextiter = iter;
				++nextiter;

				ExpressionAtomIdentifier* opstruct = dynamic_cast<ExpressionAtomIdentifier*>(*previter);
				VM::EpochTypeID structtype = activescope.GetScope()->GetVariableTypeByID(opstruct->GetIdentifier());
				StringHandle structurename = program.GetNameOfStructureType(structtype);

				ExpressionAtomIdentifier* opid = dynamic_cast<ExpressionAtomIdentifier*>(*nextiter);
				StringHandle memberaccessname = program.FindStructureMemberAccessOverload(structurename, opid->GetIdentifier());

				delete opatom;
				*iter = new ExpressionAtomOperator(memberaccessname);

				delete *nextiter;
				Atoms.erase(nextiter);

				completed = false;
				break;
			}
		}

	} while(!completed);

	InferredType = (*Atoms.rbegin())->GetEpochType(program);
}




ExpressionAtomStatement::ExpressionAtomStatement(Statement* statement)
	: MyStatement(statement)
{
}

ExpressionAtomStatement::~ExpressionAtomStatement()
{
	delete MyStatement;
}

VM::EpochTypeID ExpressionAtomStatement::GetEpochType(const Program& program) const
{
	// Hrm. Maybe return VM::EpochType_Infer until TypeInference() is called??!
	return VM::EpochType_Error;
}

bool ExpressionAtomStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return MyStatement->TypeInference(program, activescope, context);
}


ExpressionAtomParenthetical::ExpressionAtomParenthetical(Parenthetical* parenthetical)
	: MyParenthetical(parenthetical)
{
}

ExpressionAtomParenthetical::~ExpressionAtomParenthetical()
{
	delete MyParenthetical;
}

VM::EpochTypeID ExpressionAtomParenthetical::GetEpochType(const Program& program) const
{
	// TODO
	return VM::EpochType_Error;
}

bool ExpressionAtomParenthetical::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	// TODO
	return true;
}


ParentheticalPreOp::ParentheticalPreOp(PreOpStatement* statement)
	: MyStatement(statement)
{
}

ParentheticalPreOp::~ParentheticalPreOp()
{
	delete MyStatement;
}


ParentheticalPostOp::ParentheticalPostOp(PostOpStatement* statement)
	: MyStatement(statement)
{
}

ParentheticalPostOp::~ParentheticalPostOp()
{
	delete MyStatement;
}


VM::EpochTypeID ExpressionAtomIdentifier::GetEpochType(const Program& program) const
{
	// TODO
	return VM::EpochType_Error;
}

bool ExpressionAtomIdentifier::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	// TODO
	return true;
}


VM::EpochTypeID ExpressionAtomOperator::GetEpochType(const Program& program) const
{
	// TODO
	return VM::EpochType_Error;
}

bool ExpressionAtomOperator::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	// TODO
	return true;
}


VM::EpochTypeID ExpressionAtomLiteralInteger32::GetEpochType(const Program& program) const
{
	return VM::EpochType_Integer;
}

bool ExpressionAtomLiteralInteger32::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return true;
}

VM::EpochTypeID ExpressionAtomLiteralReal32::GetEpochType(const Program& program) const
{
	return VM::EpochType_Real;
}

bool ExpressionAtomLiteralReal32::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return true;
}

VM::EpochTypeID ExpressionAtomLiteralBoolean::GetEpochType(const Program& program) const
{
	return VM::EpochType_Boolean;
}

bool ExpressionAtomLiteralBoolean::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return true;
}

VM::EpochTypeID ExpressionAtomLiteralString::GetEpochType(const Program& program) const
{
	return VM::EpochType_String;
}

bool ExpressionAtomLiteralString::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return true;
}
