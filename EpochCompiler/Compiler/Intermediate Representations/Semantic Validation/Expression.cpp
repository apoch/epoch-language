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

	VM::EpochTypeID WalkAtomsForTypePartial(const std::vector<ExpressionAtom*>& atoms, Program& program, size_t& index, VM::EpochTypeID lastknowntype)
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
	newcontext.ExpectedSignatures = context.ExpectedSignatures;
	InferenceContext& selectedcontext = context.State == InferenceContext::CONTEXT_FUNCTION_RETURN ? context : newcontext;

	bool result = true;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
		if(opatom && opatom->IsOperatorUnary(program) && !opatom->IsMemberAccess())
		{
			if(!(*iter)->TypeInference(program, activescope, selectedcontext, index))
				result = false;

			newcontext.ContextName = opatom->GetIdentifier();
			newcontext.ExpectedTypes.clear();
			newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName));
			newcontext.ExpectedSignatures.clear();
			newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName));
		}
		else if(opatom && !opatom->IsMemberAccess())
		{
			if(!(*iter)->TypeInference(program, activescope, selectedcontext, index))
				result = false;

			std::vector<ExpressionAtom*>::iterator nextiter = iter;
			++nextiter;
			if(nextiter != Atoms.end())
			{
				newcontext.ContextName = opatom->GetIdentifier();
				newcontext.ExpectedTypes.clear();
				newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName));
				newcontext.ExpectedSignatures.clear();
				newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(opatom->GetIdentifier(), *activescope.GetScope(), context.ContextName));

				if(!(*nextiter)->TypeInference(program, activescope, newcontext, 1))
					result = false;

				iter = nextiter;
			}

			continue;
		}

		std::vector<ExpressionAtom*>::iterator nextiter = iter;
		++nextiter;
		if(nextiter != Atoms.end())
		{
			ExpressionAtomOperator* nextopatom = dynamic_cast<ExpressionAtomOperator*>(*nextiter);
			if(nextopatom && !nextopatom->IsMemberAccess() && !nextopatom->IsOperatorUnary(program))
			{
				InferenceContext atomcontext(nextopatom->GetIdentifier(), InferenceContext::CONTEXT_EXPRESSION);
				atomcontext.FunctionName = context.FunctionName;
				atomcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(nextopatom->GetIdentifier(), *activescope.GetScope(), context.ContextName));
				atomcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(nextopatom->GetIdentifier(), *activescope.GetScope(), context.ContextName));
				if(!(*iter)->TypeInference(program, activescope, atomcontext, 0))
					result = false;
			}
		}
		else
		{
			if(!(*iter)->TypeInference(program, activescope, selectedcontext, index))
				result = false;
		}
	}

	// Perform operator overload resolution
	unsigned idx = 0;
	for(std::vector<ExpressionAtom*>::iterator iter = Atoms.begin(); iter != Atoms.end(); ++iter)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(*iter);
		if(!opatom || opatom->IsMemberAccess())
			continue;

		OverloadMap::const_iterator overloadmapiter = program.Session.FunctionOverloadNames.find(opatom->GetIdentifier());

		VM::EpochTypeID typerhs = VM::EpochType_Error;
		unsigned rhsidx = iter - Atoms.begin() + 1;
		typerhs = WalkAtomsForType(Atoms, program, rhsidx, typerhs);

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
				VM::EpochTypeID typelhs = VM::EpochType_Error;
				typelhs = WalkAtomsForTypePartial(Atoms, program, idx, typelhs);

				bool found = false;
				const StringHandleSet& overloads = overloadmapiter->second;
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& overloadsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
					if(overloadsig.GetNumParameters() == 2)
					{
						if(overloadsig.GetParameter(0).Type == typelhs && overloadsig.GetParameter(1).Type == typerhs)
						{
							opatom->SetIdentifier(*overloaditer);
							found = true;
							break;
						}
					}
				}

				if(!found)
					return false;
			}
		}
	}

	if(!result)
	{
		UI::OutputStream output;
		output << UI::lightred << "Expression atoms failed type inference" << UI::white << std::endl;
		return false;
	}

	InferredType = VM::EpochType_Void;
	size_t i = 0;
	while(i < Atoms.size())
		InferredType = WalkAtomsForType(Atoms, program, i, InferredType);

	result = (InferredType != VM::EpochType_Infer && InferredType != VM::EpochType_Error);
	if(!result)
	{
		UI::OutputStream output;
		output << UI::lightred << "Expression failed type inference" << UI::white << std::endl;
	}

	// Perform operator precedence reordering
	// TODO - proper operator precedence, right now this is just a hacky postfix reorder
	for(size_t i = 0; i < Atoms.size(); ++i)
	{
		ExpressionAtomOperator* opatom = dynamic_cast<ExpressionAtomOperator*>(Atoms[i]);
		if(!opatom || opatom->IsMemberAccess())
			continue;

		Atoms.erase(Atoms.begin() + i);
		if(dynamic_cast<ExpressionAtomIdentifierReference*>(Atoms[i++]))
		{
			ExpressionAtomOperator* nextopatom = dynamic_cast<ExpressionAtomOperator*>(Atoms[i]);
			while(nextopatom && nextopatom->IsMemberAccess() && ++i < Atoms.size())
				nextopatom = dynamic_cast<ExpressionAtomOperator*>(Atoms[i]);
		}
	
		Atoms.insert(Atoms.begin() + i, opatom);
	}

	return result;
}

VM::EpochTypeID Expression::GetEpochType(const Program&) const
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
	VM::EpochTypeID structuretype = VM::EpochType_Error;
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

				ExpressionAtomIdentifier* identifieratom = dynamic_cast<ExpressionAtomIdentifier*>(*previter);
				if(identifieratom)
				{
					structuretype = activescope.GetScope()->GetVariableTypeByID(identifieratom->GetIdentifier());
					*previter = new ExpressionAtomIdentifierReference(identifieratom->GetIdentifier());
					delete identifieratom;

					StringHandle structurename = program.GetNameOfStructureType(structuretype);

					ExpressionAtomIdentifier* opid = dynamic_cast<ExpressionAtomIdentifier*>(*nextiter);
					StringHandle memberaccessname = program.FindStructureMemberAccessOverload(structurename, opid->GetIdentifier());

					delete opatom;
					*iter = new ExpressionAtomOperator(memberaccessname, true);

					InferenceContext newcontext(0, InferenceContext::CONTEXT_GLOBAL);
					program.GetFunctions().find(memberaccessname)->second->TypeInference(program, newcontext);
					structuretype = program.GetFunctions().find(memberaccessname)->second->GetReturnType(program);

					delete *nextiter;
					Atoms.erase(nextiter);
				}
				else
				{
					StringHandle structurename = program.GetNameOfStructureType(structuretype);

					ExpressionAtomIdentifier* opid = dynamic_cast<ExpressionAtomIdentifier*>(*nextiter);
					StringHandle memberaccessname = program.FindStructureMemberAccessOverload(structurename, opid->GetIdentifier());

					InferenceContext newcontext(0, InferenceContext::CONTEXT_GLOBAL);
					program.GetFunctions().find(memberaccessname)->second->TypeInference(program, newcontext);
					structuretype = program.GetFunctions().find(memberaccessname)->second->GetReturnType(program);

					delete opatom;
					*iter = new ExpressionAtomBindReference(opid->GetIdentifier(), structuretype);

					delete *nextiter;
					Atoms.erase(nextiter);
				}

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

bool ExpressionAtomParenthetical::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t)
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

bool ParentheticalPreOp::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
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

bool ParentheticalPostOp::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
{
	return true;
}


VM::EpochTypeID ExpressionAtomIdentifier::GetEpochType(const Program&) const
{
	return MyType;
}

bool ExpressionAtomIdentifier::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index)
{
	StringHandle resolvedidentifier = Identifier;
	VM::EpochTypeID vartype = activescope.GetScope()->HasVariable(Identifier) ? activescope.GetScope()->GetVariableTypeByID(Identifier) : VM::EpochType_Infer;
	
	std::set<VM::EpochTypeID> possibletypes;
	unsigned signaturematches = 0;

	if(!context.ExpectedTypes.empty())
	{
		const InferenceContext::PossibleParameterTypes& types = context.ExpectedTypes.back();
		for(size_t i = 0; i < types.size(); ++i)
		{
			VM::EpochTypeID paramtype = types[i][index];
			if(paramtype == VM::EpochType_Identifier)
			{
				possibletypes.insert(VM::EpochType_Identifier);
			}
			else if(paramtype == VM::EpochType_Function)
			{
				FunctionSignatureSet::const_iterator fsigiter = program.Session.FunctionSignatures.find(Identifier);
				if(fsigiter != program.Session.FunctionSignatures.end())
				{
					if(context.ExpectedSignatures.back()[i][index].Matches(fsigiter->second))
					{
						possibletypes.insert(VM::EpochType_Function);
						++signaturematches;
					}
				}
				else if(program.HasFunction(Identifier))
				{
					unsigned overloadcount = program.GetNumFunctionOverloads(Identifier);
					for(unsigned j = 0; j < overloadcount; ++j)
					{
						StringHandle overloadname = program.GetFunctionOverloadName(Identifier, j);
						Function* func = program.GetFunctions().find(overloadname)->second;

						func->TypeInference(program, context);
						FunctionSignature sig = func->GetFunctionSignature(program);
						if(context.ExpectedSignatures.back()[i][index].Matches(sig))
						{
							resolvedidentifier = overloadname;
							possibletypes.insert(VM::EpochType_Function);
							++signaturematches;
						}
					}
				}
			}
			else if(paramtype == vartype)
			{
				possibletypes.insert(vartype);
			}
		}
	}

	if(possibletypes.size() != 1)
		MyType = vartype;		// This will correctly handle structure types and fall back to Infer if all else fails
	else
	{
		if(*possibletypes.begin() == VM::EpochType_Function)
		{
			if(signaturematches == 1)
			{
				Identifier = resolvedidentifier;
				MyType = VM::EpochType_Function;
			}
			else
				MyType = vartype;
		}
		else
			MyType = *possibletypes.begin();
	}

	bool success = (MyType != VM::EpochType_Infer && MyType != VM::EpochType_Error);
	if(!success)
	{
		UI::OutputStream output;
		output << UI::lightred << L"Cannot infer type of identifier \"" << program.GetString(Identifier) << L"\"" << UI::white << std::endl;
	}

	return success;
}

bool ExpressionAtomIdentifier::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
{
	// No op
	return true;
}


VM::EpochTypeID ExpressionAtomOperator::GetEpochType(const Program&) const
{
	return VM::EpochType_Error;
}

bool ExpressionAtomOperator::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t)
{
	return true;
}

bool ExpressionAtomOperator::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
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


VM::EpochTypeID ExpressionAtomLiteralInteger32::GetEpochType(const Program&) const
{
	return VM::EpochType_Integer;
}

bool ExpressionAtomLiteralInteger32::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t)
{
	return true;
}

bool ExpressionAtomLiteralInteger32::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
{
	// No op
	return true;
}

CompileTimeParameter ExpressionAtomLiteralInteger32::ConvertToCompileTimeParam(const Program&) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_Integer);
	ret.Payload.IntegerValue = Value;
	return ret;
}

VM::EpochTypeID ExpressionAtomLiteralReal32::GetEpochType(const Program&) const
{
	return VM::EpochType_Real;
}

bool ExpressionAtomLiteralReal32::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t)
{
	return true;
}

bool ExpressionAtomLiteralReal32::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
{
	// No op
	return true;
}

CompileTimeParameter ExpressionAtomLiteralReal32::ConvertToCompileTimeParam(const Program&) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_Real);
	ret.Payload.RealValue = Value;
	return ret;
}

VM::EpochTypeID ExpressionAtomLiteralBoolean::GetEpochType(const Program&) const
{
	return VM::EpochType_Boolean;
}

bool ExpressionAtomLiteralBoolean::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t)
{
	return true;
}

bool ExpressionAtomLiteralBoolean::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
{
	// No op
	return true;
}

CompileTimeParameter ExpressionAtomLiteralBoolean::ConvertToCompileTimeParam(const Program&) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_Boolean);
	ret.Payload.BooleanValue = Value;
	return ret;
}



VM::EpochTypeID ExpressionAtomLiteralString::GetEpochType(const Program&) const
{
	return VM::EpochType_String;
}

bool ExpressionAtomLiteralString::TypeInference(Program&, CodeBlock&, InferenceContext&, size_t)
{
	return true;
}

bool ExpressionAtomLiteralString::CompileTimeCodeExecution(Program&, CodeBlock&, bool)
{
	// No op
	return true;
}

CompileTimeParameter ExpressionAtomLiteralString::ConvertToCompileTimeParam(const Program& program) const
{
	CompileTimeParameter ret(L"@@autoctp", VM::EpochType_String);
	ret.Payload.LiteralStringHandleValue = Handle;
	ret.StringPayload = program.GetString(Handle);
	return ret;
}
