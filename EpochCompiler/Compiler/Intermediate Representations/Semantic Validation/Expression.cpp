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
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Session.h"

#include "User Interface/Output.h"



using namespace IRSemantics;


Expression::Expression()
	: InferredType(VM::EpochType_Error),
	  Coalesced(false)
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
			++program.Session.ErrorCount;
		}
		return false;

	case VM::EpochType_Infer:
		{
			UI::OutputStream output;
			output << L"Type inference failed or otherwise incomplete" << std::endl;
			++program.Session.ErrorCount;
		}
		return false;
	}

	return true;
}


bool Expression::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	Coalesce(program, activescope);

	bool result = true;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, activescope, inreturnexpr))
			result = false;
	}

	return result;
}


namespace
{
	VM::EpochTypeID WalkAtomsForType(const std::vector<ExpressionAtom*>& atoms, Program& program, size_t& index, VM::EpochTypeID lastknowntype)
	{
		VM::EpochTypeID ret = lastknowntype;

		while(index < atoms.size())
		{
			if(ret == VM::EpochType_Infer)
			{
				index = atoms.size();
				break;
			}

			const ExpressionAtomOperator* opatom = dynamic_cast<const ExpressionAtomOperator*>(atoms[index]);
			if(opatom)
			{
				if(opatom->IsMemberAccess())
				{
					Function* func = program.GetFunctions().find(opatom->GetIdentifier())->second;
					InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
					func->TypeInference(program, context);
					ret = func->GetReturnType(program);
					++index;
				}
				else if(opatom->IsOperatorUnary(program))
				{
					VM::EpochTypeID operandtype = WalkAtomsForType(atoms, program, ++index, ret);
					if(operandtype == VM::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					ret = opatom->DetermineUnaryReturnType(program, operandtype);
				}
				else
				{
					VM::EpochTypeID rhstype = WalkAtomsForType(atoms, program, ++index, ret);
					if(rhstype == VM::EpochType_Infer)
					{
						index = atoms.size();
						break;
					}

					ret = opatom->DetermineOperatorReturnType(program, ret, rhstype);
				}

				break;
			}
			else
				ret = atoms[index++]->GetEpochType(program);
		}

		return ret;
	}
}


bool Expression::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	Coalesce(program, activescope);

	InferenceContext newcontext(context.ContextName, InferenceContext::CONTEXT_EXPRESSION);
	newcontext.FunctionName = context.FunctionName;
	newcontext.ExpectedTypes = context.ExpectedTypes;
	InferenceContext& selectedcontext = context.State == InferenceContext::CONTEXT_FUNCTION_RETURN ? context : newcontext;

	bool result = true;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, selectedcontext, index))
			result = false;
	}

	if(!result)
		return false;

	// Perform operator precedence reordering
	// TODO - proper operator precedence, right now this is just a hacky postfix reorder
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
		if(!opatom || opatom->IsMemberAccess())
			continue;

		OverloadMap::const_iterator overloadmapiter = program.Session.FunctionOverloadNames.find(opatom->GetIdentifier());

		std::vector<ExpressionAtom*>::iterator nextiter = iter;
		++nextiter;
		
		VM::EpochTypeID typerhs = (*nextiter)->GetEpochType(program);

		if(program.Session.InfoTable.UnaryPrefixes->find(program.GetString(opatom->GetIdentifier())) != program.Session.InfoTable.UnaryPrefixes->end())
		{
			if(overloadmapiter != program.Session.FunctionOverloadNames.end())
			{
				const StringHandleSet& overloads = overloadmapiter->second;
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& overloadsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
					if(overloadsig.GetNumParameters() == 1)
					{
						if(overloadsig.GetParameter(0).Type == typerhs)
						{
							opatom->SetIdentifier(*overloaditer);
							break;
						}
					}
				}
			}
		}
		else
		{
			if(overloadmapiter != program.Session.FunctionOverloadNames.end())
			{
				std::vector<ExpressionAtom*>::iterator previter = iter;
				--previter;

				VM::EpochTypeID typelhs = (*previter)->GetEpochType(program);

				const StringHandleSet& overloads = overloadmapiter->second;
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& overloadsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
					if(overloadsig.GetNumParameters() == 2)
					{
						if(overloadsig.GetParameter(0).Type == typelhs && overloadsig.GetParameter(1).Type == typerhs)
						{
							opatom->SetIdentifier(*overloaditer);
							break;
						}
					}
				}
			}
		}

		std::swap(*iter, *nextiter);

		iter = nextiter;
	}

	InferredType = VM::EpochType_Void;
	size_t i = 0;
	while(i < Atoms.size())
		InferredType = WalkAtomsForType(Atoms, program, i, InferredType);

	return true;
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
	if(Coalesced)
		return;

	Coalesced = true;

	if(Atoms.empty())
		return;

	// Flatten member accesses
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
				*iter = new ExpressionAtomOperator(memberaccessname, true);

				delete *nextiter;
				Atoms.erase(nextiter);

				completed = false;
				break;
			}
		}

	} while(!completed);
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
	return MyStatement->GetEpochType(program);
}

bool ExpressionAtomStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return MyStatement->TypeInference(program, activescope, context, index);
}

bool ExpressionAtomStatement::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	return MyStatement->CompileTimeCodeExecution(program, activescope, inreturnexpr);
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
	if(MyParenthetical)
		return MyParenthetical->GetEpochType(program);

	return VM::EpochType_Error;
}

bool ExpressionAtomParenthetical::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return MyParenthetical->TypeInference(program, activescope, context);
}

bool ExpressionAtomParenthetical::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	return MyParenthetical->CompileTimeCodeExecution(program, activescope, inreturnexpr);
}


ParentheticalPreOp::ParentheticalPreOp(PreOpStatement* statement)
	: MyStatement(statement)
{
}

ParentheticalPreOp::~ParentheticalPreOp()
{
	delete MyStatement;
}

VM::EpochTypeID ParentheticalPreOp::GetEpochType(const Program& program) const
{
	return MyStatement->GetEpochType(program);
}

bool ParentheticalPreOp::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context) const
{
	return MyStatement->TypeInference(program, activescope, context);
}

bool ParentheticalPreOp::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	return true;
}


ParentheticalPostOp::ParentheticalPostOp(PostOpStatement* statement)
	: MyStatement(statement)
{
}

ParentheticalPostOp::~ParentheticalPostOp()
{
	delete MyStatement;
}

VM::EpochTypeID ParentheticalPostOp::GetEpochType(const Program& program) const
{
	return MyStatement->GetEpochType(program);
}

bool ParentheticalPostOp::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context) const
{
	return MyStatement->TypeInference(program, activescope, context);
}

bool ParentheticalPostOp::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	return true;
}


VM::EpochTypeID ExpressionAtomIdentifier::GetEpochType(const Program& program) const
{
	return MyType;
}

bool ExpressionAtomIdentifier::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	if(program.HasFunction(Identifier))
		MyType = VM::EpochType_Function;
	else if(program.LookupType(Identifier) != VM::EpochType_Error)
		MyType = VM::EpochType_Identifier;
	else
	{
		// TODO - enumerate possible types and perform actual inference
		if(!context.ExpectedTypes.empty() && context.ExpectedTypes.back()[0][index] == VM::EpochType_Identifier)
			MyType = VM::EpochType_Identifier;
		else if(!context.ExpectedTypes.empty() && context.ExpectedTypes.back()[0][index] == VM::EpochType_Function)
			MyType = VM::EpochType_Function;
		else
			MyType = activescope.GetScope()->GetVariableTypeByID(Identifier);
	}

	return true;
}

bool ExpressionAtomIdentifier::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	// No op
	return true;
}


VM::EpochTypeID ExpressionAtomOperator::GetEpochType(const Program& program) const
{
	return VM::EpochType_Error;
}

bool ExpressionAtomOperator::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return true;
}

bool ExpressionAtomOperator::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	// No op
	return true;
}

bool ExpressionAtomOperator::IsOperatorUnary(const Program& program) const
{
	if(program.HasFunction(Identifier))
		return (program.GetFunctions().find(Identifier)->second->GetNumParameters() == 1);

	OverloadMap::const_iterator ovmapiter = program.Session.FunctionOverloadNames.find(Identifier);
	if(ovmapiter != program.Session.FunctionOverloadNames.end())
	{
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(*oviter);
			if(funcsigiter != program.Session.FunctionSignatures.end())
			{
				if(funcsigiter->second.GetNumParameters() == 1)
					return true;	
			}
		}
	}

	FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(Identifier);
	if(funcsigiter != program.Session.FunctionSignatures.end())
	{
		if(funcsigiter->second.GetNumParameters() == 1)
			return true;
	}

	return false;
}

VM::EpochTypeID ExpressionAtomOperator::DetermineOperatorReturnType(Program& program, VM::EpochTypeID lhstype, VM::EpochTypeID rhstype) const
{
	if(program.HasFunction(Identifier))
	{
		Function* func = program.GetFunctions().find(Identifier)->second;
		InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
		func->TypeInference(program, context);
		return func->GetReturnType(program);
	}

	OverloadMap::const_iterator ovmapiter = program.Session.FunctionOverloadNames.find(Identifier);
	if(ovmapiter != program.Session.FunctionOverloadNames.end())
	{
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(*oviter);
			if(funcsigiter != program.Session.FunctionSignatures.end())
			{
				if(funcsigiter->second.GetNumParameters() == 2)
				{
					if(funcsigiter->second.GetParameter(0).Type == lhstype && funcsigiter->second.GetParameter(1).Type == rhstype)
						return funcsigiter->second.GetReturnType();
				}
			}
		}
	}

	FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(Identifier);
	if(funcsigiter != program.Session.FunctionSignatures.end())
	{
		if(funcsigiter->second.GetNumParameters() == 2)
		{
			if(funcsigiter->second.GetParameter(0).Type == lhstype && funcsigiter->second.GetParameter(1).Type == rhstype)
				return funcsigiter->second.GetReturnType();
		}
	}

	return VM::EpochType_Error;
}

VM::EpochTypeID ExpressionAtomOperator::DetermineUnaryReturnType(Program& program, VM::EpochTypeID operandtype) const
{
	if(program.HasFunction(Identifier))
	{
		Function* func = program.GetFunctions().find(Identifier)->second;
		InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);
		func->TypeInference(program, context);
		return func->GetReturnType(program);
	}

	OverloadMap::const_iterator ovmapiter = program.Session.FunctionOverloadNames.find(Identifier);
	if(ovmapiter != program.Session.FunctionOverloadNames.end())
	{
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(*oviter);
			if(funcsigiter != program.Session.FunctionSignatures.end())
			{
				if(funcsigiter->second.GetNumParameters() == 1)
				{
					if(funcsigiter->second.GetParameter(0).Type == operandtype)
						return funcsigiter->second.GetReturnType();
				}
			}
		}
	}

	FunctionSignatureSet::const_iterator funcsigiter = program.Session.FunctionSignatures.find(Identifier);
	if(funcsigiter != program.Session.FunctionSignatures.end())
	{
		if(funcsigiter->second.GetNumParameters() == 1)
		{
			if(funcsigiter->second.GetParameter(0).Type == operandtype)
				return funcsigiter->second.GetReturnType();
		}
	}

	return VM::EpochType_Error;
}


VM::EpochTypeID ExpressionAtomLiteralInteger32::GetEpochType(const Program& program) const
{
	return VM::EpochType_Integer;
}

bool ExpressionAtomLiteralInteger32::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	return true;
}

bool ExpressionAtomLiteralInteger32::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	// No op
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

bool ExpressionAtomLiteralReal32::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	// No op
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

bool ExpressionAtomLiteralBoolean::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	// No op
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

bool ExpressionAtomLiteralString::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr)
{
	// No op
	return true;
}
