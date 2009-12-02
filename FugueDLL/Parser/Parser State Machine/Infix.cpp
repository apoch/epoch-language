//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Infix operator-related routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/SelfAware.inl"

#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Operations/Variables/StringOps.h"
#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/Operators/CompoundOperator.h"
#include "Virtual Machine/Operations/Operators/Bitwise.h"
#include "Virtual Machine/Operations/Operators/Logical.h"

#include "Virtual Machine/Types Management/TypeInfo.h"

#include "Utility/Strings.h"


using namespace Parser;


// Defined operator precedence levels
enum OperatorPrecedence
{
	OPREC_MIN,
	OPREC_ASSIGNMENT,
	OPREC_BITWISE,
	OPREC_LOGICAL,
	OPREC_EQUALITY,
	OPREC_COMPARISON,
	OPREC_USER,
	OPREC_CALCASSIGN,
	OPREC_ADDITION,
	OPREC_MULTIPLICATION,
	OPREC_BOOLEAN,
	OPREC_CONCATENATION,
	OPREC_INCREMENT,
	OPREC_MEMBER,
	OPREC_MAX
};


//
// Record for tracking details about an infix operator
//
struct InfixOperatorData
{
	OperatorPrecedence Precedence;
	std::wstring FunctionName;
};

// Internal tracker of all known infix operators (including user defined ops)
namespace { std::map<std::wstring, InfixOperatorData> InfixOperators; }


//
// Add an infix operator to the current infix expression
//
void ParserState::PushInfixOperator(const std::wstring& opname)
{
	if(!InfixOperatorList.empty())
		InfixOperatorList.top().push_back(opname);
}

//
// Track the presence of an operand in the current infix expression
//
void ParserState::RegisterInfixOperand()
{
	if(!InfixOperandCount.empty())
		++InfixOperandCount.back();
}

//
// Register that the current infix operation is being used as an l-value
//
void ParserState::RegisterInfixOperandAsLValue(const std::wstring& lvaluename)
{
	RegisterInfixOperand();

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_IDENTIFIER;
	entry.StringValue = StripWhitespace(lvaluename.substr(0, lvaluename.find(Operators::Assign)));
	TheStack.push_back(entry);

	InfixOperatorList.push(std::list<std::wstring>());
	InfixOperatorList.top().push_back(Operators::Assign);

	PassedParameterCount.push(0);
	InfixOperandCount.push_back(0);
}

//
// Finish parsing an infix expression and generate the associated operations
// Note that this may take several passes to ensure that all the ops are
// set up correctly.
//
void ParserState::TerminateInfixExpression()
{
	ResetMemberAccess();

	bool firstrun = true;
	while(FinalizeInfixExpression(firstrun, *CurrentScope))
		firstrun = false;

	LastMemberLevelRValue = MemberLevelRValue;
	MemberLevelRValue = 0;
}



//
// Helper classes for working with infix expressions
//
struct InfixUnit
{
	virtual ~InfixUnit()
	{ }

	virtual void PushContents(VM::Block* block) const = 0;
	virtual void PushOperandsToStack(std::deque<ParserState::StackEntry>& opstack) const = 0;
	virtual void ClearOperands() = 0;
	virtual void ClearOperations() = 0;
	virtual void CopyInstructionsToOp(VM::Operation* op) const = 0;
};

struct InfixUnitRawOperations : public InfixUnit
{
	std::list<VM::Operation*> PushOperations;
	std::list<ParserState::StackEntry> Operands;

	virtual void PushContents(VM::Block* block) const
	{
		for(std::list<VM::Operation*>::const_iterator iter = PushOperations.begin(); iter != PushOperations.end(); ++iter)
			block->AddOperation(VM::OperationPtr(*iter));
	}

	virtual void PushOperandsToStack(std::deque<ParserState::StackEntry>& opstack) const
	{
		for(std::list<ParserState::StackEntry>::const_iterator iter = Operands.begin(); iter != Operands.end(); ++iter)
			opstack.push_back(*iter);
	}

	virtual void ClearOperands()
	{
		Operands.clear();
	}

	virtual void ClearOperations()
	{
		for(std::list<VM::Operation*>::iterator iter = PushOperations.begin(); iter != PushOperations.end(); ++iter)
		{
			VM::Operation* op = *iter;
			VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op);
			if(pushop)
				pushop->UnlinkOperation();

			delete op;
		}

		PushOperations.clear();
	}

	virtual void CopyInstructionsToOp(VM::Operation* op) const
	{
		VM::Operations::CompoundOperator* compoperator = dynamic_cast<VM::Operations::CompoundOperator*>(op);
		if(compoperator)
		{
			for(std::list<VM::Operation*>::const_iterator iter = PushOperations.begin(); iter != PushOperations.end(); ++iter)
				compoperator->AddOperation(*iter);
		}
	}
};

struct InfixUnitCompound : public InfixUnit
{
	std::list<InfixUnit*> Units;

	virtual ~InfixUnitCompound()
	{
		for(std::list<InfixUnit*>::const_iterator iter = Units.begin(); iter != Units.end(); ++iter)
			delete (*iter);
	}

	virtual void PushContents(VM::Block* block) const
	{
		for(std::list<InfixUnit*>::const_iterator iter = Units.begin(); iter != Units.end(); ++iter)
			(*iter)->PushContents(block);
	}

	virtual void PushOperandsToStack(std::deque<ParserState::StackEntry>& opstack) const
	{
		for(std::list<InfixUnit*>::const_iterator iter = Units.begin(); iter != Units.end(); ++iter)
			(*iter)->PushOperandsToStack(opstack);
	}

	virtual void ClearOperands()
	{
		for(std::list<InfixUnit*>::const_iterator iter = Units.begin(); iter != Units.end(); ++iter)
			(*iter)->ClearOperands();
	}

	virtual void ClearOperations()
	{
		for(std::list<InfixUnit*>::const_iterator iter = Units.begin(); iter != Units.end(); ++iter)
			(*iter)->ClearOperations();
	}

	virtual void CopyInstructionsToOp(VM::Operation* op) const
	{
		for(std::list<InfixUnit*>::const_iterator iter = Units.begin(); iter != Units.end(); ++iter)
			(*iter)->CopyInstructionsToOp(op);
	}
};

//
// Run a pass on the current infix expression and reduce it to the
// corresponding operation sequence
//
bool ParserState::FinalizeInfixExpression(bool isfirstrun, const VM::ScopeDescription& scope)
{
	if(InfixOperatorList.empty() || InfixOperandCount.empty())
		return false;

	if(InfixOperatorList.top().empty() || (InfixOperandCount.back() == 1 && InfixOperatorList.top().back() != Operators::Assign))
	{
		InfixOperatorList.pop();
		InfixOperandCount.pop_back();
		return false;
	}

	if(InfixOperandCount.back() == 1 && InfixOperatorList.top().back() == Operators::Assign && !isfirstrun)
	{
		if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		{
			ReportFatalError("Expected a variable name on the left side of the assignment operator");
			return false;
		}

		std::wstring lvaluename = TheStack.back().StringValue;
		std::wstring previouslvaluename = dynamic_cast<VM::Operations::AssignValue*>(Blocks.back().TheBlock->GetTailOperation())->GetAssociatedIdentifier();

		TheStack.pop_back();

		VM::OperationPtr getvalop(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(previouslvaluename)));
		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushOperation(getvalop.release())));
		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(lvaluename))));

		InfixOperandCount.pop_back();
		InfixOperatorList.pop();

		return true;
	}


	struct safety
	{
		std::list<InfixUnit*> units;

		~safety()
		{
			for(std::list<InfixUnit*>::iterator iter = units.begin(); iter != units.end(); ++iter)
				delete *iter;
		}
	} unitssafety;

	VM::EpochVariableTypeID expressiontype = Blocks.back().TheBlock->GetTailOperation()->GetType(*CurrentScope);

	// First we convert the stream of infix operands into a sequence of infix units
	bool bailout = false;
	for(unsigned i = 0; i < InfixOperandCount.back(); ++i)
	{
		std::auto_ptr<InfixUnitRawOperations> unit(new InfixUnitRawOperations);
		size_t numops = Blocks.back().TheBlock->GetNumOperations() - Blocks.back().TheBlock->CountTailOps(1, scope);
		for(unsigned j = 0; j < numops; ++j)
		{
			VM::OperationPtr op(Blocks.back().TheBlock->PopTailOperation());

			VM::EpochVariableTypeID optype = op->GetType(*CurrentScope);
			if(!bailout && optype != expressiontype)
			{
				if(optype == VM::EpochVariableType_List)
				{
					VM::Operations::ConsList* consop;
					VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op.get());
					if(pushop)
						consop = dynamic_cast<VM::Operations::ConsList*>(pushop->GetNestedOperation());
					else
						consop = dynamic_cast<VM::Operations::ConsList*>(op.get());

					if(!consop || consop->GetElementType() != expressiontype)
					{
						ReportFatalError("All values in the expression must be of the same type");
						bailout = true;
					}
				}
			}

			unit->PushOperations.push_front(op.release());
		}

		unit->Operands.push_front(TheStack.back());
		TheStack.pop_back();

		unitssafety.units.push_front(unit.release());
	}

	if(bailout)
	{
		while(!InfixOperatorList.empty())
		{
			unsigned assigncount = 0;
			for(std::list<std::wstring>::const_iterator iter = InfixOperatorList.top().begin(); iter != InfixOperatorList.top().end(); ++iter)
			{
				if(*iter == Operators::Assign)
					++assigncount;
			}

			for(unsigned i = 0; i < assigncount; ++i)
				TheStack.pop_back();

			InfixOperatorList.pop();
		}

		InfixOperandCount.clear();

		return false;
	}

	bool ret = false;
	std::wstring injectlvalue;

	// Now we combine units using operator precedence rules to determine
	// which operands go together with which operations
	for(unsigned precedence = OPREC_MAX - 1; precedence > OPREC_MIN; --precedence)
	{
		std::list<InfixUnit*>::iterator unititer = unitssafety.units.begin();
		for(std::list<std::wstring>::iterator iter = InfixOperatorList.top().begin(); iter != InfixOperatorList.top().end(); )
		{
			if(GetInfixPrecedence(*iter) == precedence)
			{
				if(*iter == Operators::Assign)
				{
					injectlvalue = TheStack.back().StringValue;
					TheStack.pop_back();
					ret = true;

					if(CurrentScope->GetVariableType(injectlvalue) != expressiontype)
					{
						ReportFatalError("Variable must have the same type as the expression");
						return false;
					}
				}
				else
				{
					bool discardoperations = false;

					InfixUnit* first = *unititer;
					unititer = unitssafety.units.erase(unititer);
					InfixUnit* second = *unititer;

					unsigned actualparametercount = 2;
					std::swap(actualparametercount, PassedParameterCount.top());

					// For some operators, CreateOperation requires that there be instructions on the code block
					// that set up the values passed to the operation. We have stripped those instructions above
					// and therefore must create the operation ourselves.
					VM::OperationPtr op(NULL);
					std::wstring opname = LookupInfixAlias(*iter);
					if(opname == Keywords::Or)
					{
						discardoperations = true;
						if(expressiontype == VM::EpochVariableType_Integer)
							op.reset(new VM::Operations::BitwiseOr);
						else if(expressiontype == VM::EpochVariableType_Boolean)
							op.reset(new VM::Operations::LogicalOr);
						else
							throw ParserFailureException("Invalid type for boolean operator");
					}
					else if(opname == Keywords::And)
					{
						discardoperations = true;
						if(expressiontype == VM::EpochVariableType_Integer)
							op.reset(new VM::Operations::BitwiseAnd);
						else if(expressiontype == VM::EpochVariableType_Boolean)
							op.reset(new VM::Operations::LogicalAnd);
						else
							throw ParserFailureException("Invalid type for boolean operator");
					}
					else
					{
						first->PushOperandsToStack(TheStack);
						second->PushOperandsToStack(TheStack);

						op.reset(CreateOperation(opname).release());
					}

					VM::Operation* originalop = op.get();
					VM::OperationPtr pushop(new VM::Operations::PushOperation(op.release()));
					std::auto_ptr<InfixUnitRawOperations> pushopunit(new InfixUnitRawOperations);
					pushopunit->PushOperations.push_back(pushop.release());

					first->ClearOperands();

					std::auto_ptr<InfixUnitCompound> newunit(new InfixUnitCompound);
					newunit->Units.push_back(first);
					newunit->Units.push_back(second);

					newunit->CopyInstructionsToOp(originalop);
					if(discardoperations)
						newunit->ClearOperations();

					newunit->Units.push_back(pushopunit.release());

					(*unititer) = newunit.release();

					--actualparametercount;
					std::swap(actualparametercount, PassedParameterCount.top());
				}

				iter = InfixOperatorList.top().erase(iter);
			}
			else
			{
				if((*iter) != Operators::Assign)
					++unititer;

				++iter;
			}
		}
	}

	// Finally, traverse the unit list, pushing the unit operations onto the instruction block
	for(std::list<InfixUnit*>::iterator iter = unitssafety.units.begin(); iter != unitssafety.units.end(); )
	{
		(*iter)->PushContents(Blocks.back().TheBlock);
		delete (*iter);
		iter = unitssafety.units.erase(iter);
	}

	if(!injectlvalue.empty())
	{
		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(injectlvalue))));
	}
	else
	{
		StackEntry entry;
		entry.Type = StackEntry::STACKENTRYTYPE_OPERATION;
		entry.OperationPointer = Blocks.back().TheBlock->GetTailOperation();
		TheStack.push_back(entry);
	}

	if(!InfixOperandCount.empty())
		InfixOperandCount.pop_back();

	if(!InfixOperatorList.empty())
		InfixOperatorList.pop();

	return ret;
}


//
// Register that a parenthetical infix expression was just finished
//
void ParserState::TerminateParenthetical()
{
	PassedParameterCount.pop();
	if(!PassedParameterCount.empty())
		++PassedParameterCount.top();
}

//
// Translate an infix operator's "short" name (e.g. +) into the name of
// the actual corresponding function that should be called (e.g. add)
//
std::wstring ParserState::LookupInfixAlias(const std::wstring& opname) const
{
	std::map<std::wstring, InfixOperatorData>::const_iterator iter = InfixOperators.find(opname);
	if(iter != InfixOperators.end())
		return iter->second.FunctionName;

	throw ParserFailureException("Unrecognized infix operator");
}

//
// Look up the precedence level of a given infix operator
//
unsigned ParserState::GetInfixPrecedence(const std::wstring& opname) const
{
	std::map<std::wstring, InfixOperatorData>::const_iterator iter = InfixOperators.find(opname);
	if(iter != InfixOperators.end())
		return iter->second.Precedence;

	throw ParserFailureException("Unrecognized infix operator");
}

//
// Reset the current infix expression parsing
//
void ParserState::ResetInfixTracking()
{
	InfixOperandCount.push_back(0);
	InfixOperatorList.push(std::list<std::wstring>());
}

//
// Note that we should inject a not operator before the next parameter
//
void ParserState::RegisterNotOperation()
{
	InjectNotOperator = true;
}

//
// Note that we should inject a negation operator before the next parameter
//
void ParserState::RegisterNegateOperation()
{
	InjectNegateOperator = true;
}

//
// Cancel the injection of a negation operator
// This is used to help back out of partially-matched productions in the grammar
//
void ParserState::UndoNegateOperation()
{
	InjectNegateOperator = false;
}

//
// Register an operate-and-assign operation (e.g. +=)
//
void ParserState::RegisterOpAssignment()
{
	StackEntry lvalue = TheStack.back();
	TheStack.pop_back();
	StackEntry value = TheStack.back();
	TheStack.pop_back();


	if(lvalue.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Expected a variable name on the left side");
		return;
	}

	VM::EpochVariableTypeID lefttype = CurrentScope->GetVariableType(lvalue.StringValue);
	VM::EpochVariableTypeID type = value.DetermineEffectiveType(*CurrentScope);

	if(lefttype != type)
	{
		ReportFatalError("Variable type and expression type do not match");
		return;
	}

	VM::OperationPtr readop(new VM::Operations::PushOperation(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(lvalue.StringValue))));
	AddOperationToCurrentBlock(readop);
	Blocks.back().TheBlock->ReverseTailOperations(2, *CurrentScope);

	VM::OperationPtr arithmeticop(NULL);
	if(OpAssignmentOperator == Operators::AddAssign)
	{
		switch(type)
		{
		case VM::EpochVariableType_Integer:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::SumIntegers(false, false)));			break;
		case VM::EpochVariableType_Integer16:	arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::SumInteger16s(false, false)));			break;
		case VM::EpochVariableType_Real:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::SumReals(false, false)));				break;
		default:								throw ParserFailureException("Invalid type for this operation");
		}
	}
	else if(OpAssignmentOperator == Operators::SubtractAssign)
	{
		switch(type)
		{
		case VM::EpochVariableType_Integer:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::SubtractIntegers(false, false)));		break;
		case VM::EpochVariableType_Integer16:	arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::SubtractInteger16s(false, false)));	break;
		case VM::EpochVariableType_Real:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::SubtractReals(false, false)));			break;
		default:								throw ParserFailureException("Invalid type for this operation");
		}
	}
	else if(OpAssignmentOperator == Operators::MultiplyAssign)
	{
		switch(type)
		{
		case VM::EpochVariableType_Integer:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::MultiplyIntegers(false, false)));		break;
		case VM::EpochVariableType_Integer16:	arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::MultiplyInteger16s(false, false)));	break;
		case VM::EpochVariableType_Real:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::MultiplyReals(false, false)));			break;
		default:								throw ParserFailureException("Invalid type for this operation");
		}
	}
	else if(OpAssignmentOperator == Operators::DivideAssign)
	{
		switch(type)
		{
		case VM::EpochVariableType_Integer:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::DivideIntegers(false, false)));		break;
		case VM::EpochVariableType_Integer16:	arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::DivideInteger16s(false, false)));		break;
		case VM::EpochVariableType_Real:		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::DivideReals(false, false)));			break;
		default:								throw ParserFailureException("Invalid type for this operation");
		}
	}
	else if(OpAssignmentOperator == Operators::ConcatAssign)
	{
		if(type != VM::EpochVariableType_String)
			throw ParserFailureException("Invalid type for this operation");

		arithmeticop.reset(new VM::Operations::PushOperation(new VM::Operations::Concatenate(false, false)));
	}
	else
		throw ParserFailureException("Unrecognized infix assignment operator!");

	AddOperationToCurrentBlock(arithmeticop);
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(lvalue.StringValue))));
}

//
// Track the name of the operate-and-assign operator being parsed
//
void ParserState::RegisterOpAssignmentOperator(const std::wstring& op)
{
	OpAssignmentOperator = op;

	InfixOperandCount.push_back(0);
	InfixOperatorList.push(std::list<std::wstring>());

	PassedParameterCount.push(0);
}

namespace
{

	//
	// Internal helper: set up infix operator metadata
	//
	void DefineInfixOperator(const std::wstring& operatorname, const std::wstring& functionname, OperatorPrecedence precedence)
	{
		InfixOperatorData entry;
		entry.FunctionName = functionname;
		entry.Precedence = precedence;

		InfixOperators.insert(std::make_pair(operatorname, entry));
	}

}


//
// Record that the following function is to be added to the infix operator list
//
void ParserState::RegisterUserDefinedInfix()
{
	FunctionIsInfix = true;
}

//
// Actually add a function to the infix operator table
//
void ParserState::RegisterInfixFunction(const std::wstring& functionname)
{
	DefineInfixOperator(functionname, functionname, OPREC_USER);
	UserInfixOperators.insert(narrow(functionname));
}

//
// Inject a preincrement operation
//
void ParserState::PreincrementVariable()
{
	VM::EpochVariableTypeID type = CurrentScope->GetVariableType(SavedStringSlots[SavedStringSlot_IncDec]);
	if(!TypeInfo::IsNumeric(type))
	{
		ReportFatalError("Cannot increment a non-numeric variable");
		return;
	}

	VM::OperationPtr readop(new VM::Operations::PushOperation(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
	AddOperationToCurrentBlock(readop);

	VM::OperationPtr constop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		constop.reset(new VM::Operations::PushIntegerLiteral(1));		break;
	case VM::EpochVariableType_Integer16:	constop.reset(new VM::Operations::PushInteger16Literal(1));		break;
	case VM::EpochVariableType_Real:		constop.reset(new VM::Operations::PushRealLiteral(1.0f));		break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}
	AddOperationToCurrentBlock(constop);

	VM::OperationPtr innerop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		innerop.reset(new VM::Operations::SumIntegers(false, false));		break;
	case VM::EpochVariableType_Integer16:	innerop.reset(new VM::Operations::SumInteger16s(false, false));		break;
	case VM::EpochVariableType_Real:		innerop.reset(new VM::Operations::SumReals(false, false));			break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}
	
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushOperation(innerop.release())));
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
}

//
// Inject a predecrement operation
//
void ParserState::PredecrementVariable()
{
	VM::EpochVariableTypeID type = CurrentScope->GetVariableType(SavedStringSlots[SavedStringSlot_IncDec]);
	if(!TypeInfo::IsNumeric(type))
	{
		ReportFatalError("Cannot decrement a non-numeric variable");
		return;
	}

	VM::OperationPtr readop(new VM::Operations::PushOperation(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
	AddOperationToCurrentBlock(readop);

	VM::OperationPtr constop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		constop.reset(new VM::Operations::PushIntegerLiteral(1));		break;
	case VM::EpochVariableType_Integer16:	constop.reset(new VM::Operations::PushInteger16Literal(1));		break;
	case VM::EpochVariableType_Real:		constop.reset(new VM::Operations::PushRealLiteral(1.0f));		break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}
	AddOperationToCurrentBlock(constop);

	VM::OperationPtr innerop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		innerop.reset(new VM::Operations::SubtractIntegers(false, false));		break;
	case VM::EpochVariableType_Integer16:	innerop.reset(new VM::Operations::SubtractInteger16s(false, false));	break;
	case VM::EpochVariableType_Real:		innerop.reset(new VM::Operations::SubtractReals(false, false));			break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}

	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushOperation(innerop.release())));
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
}

//
// Inject a postincrement operation
//
void ParserState::PostincrementVariable()
{
	VM::EpochVariableTypeID type = CurrentScope->GetVariableType(SavedStringSlots[SavedStringSlot_IncDec]);
	if(!TypeInfo::IsNumeric(type))
	{
		ReportFatalError("Cannot increment a non-numeric variable");
		return;
	}

	VM::OperationPtr readop(new VM::Operations::PushOperation(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
	AddOperationDeferred(readop);

	VM::OperationPtr constop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		constop.reset(new VM::Operations::PushIntegerLiteral(1));		break;
	case VM::EpochVariableType_Integer16:	constop.reset(new VM::Operations::PushInteger16Literal(1));		break;
	case VM::EpochVariableType_Real:		constop.reset(new VM::Operations::PushRealLiteral(1.0f));		break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}
	AddOperationDeferred(constop);

	VM::OperationPtr innerop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		innerop.reset(new VM::Operations::SumIntegers(false, false));		break;
	case VM::EpochVariableType_Integer16:	innerop.reset(new VM::Operations::SumInteger16s(false, false));		break;
	case VM::EpochVariableType_Real:		innerop.reset(new VM::Operations::SumReals(false, false));			break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}

	AddOperationDeferred(VM::OperationPtr(new VM::Operations::PushOperation(innerop.release())));
	AddOperationDeferred(VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
}

//
// Inject a postdecrement operation
//
void ParserState::PostdecrementVariable()
{
	VM::EpochVariableTypeID type = CurrentScope->GetVariableType(SavedStringSlots[SavedStringSlot_IncDec]);
	if(!TypeInfo::IsNumeric(type))
	{
		ReportFatalError("Cannot decrement a non-numeric variable");
		return;
	}

	VM::OperationPtr readop(new VM::Operations::PushOperation(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
	AddOperationDeferred(readop);

	VM::OperationPtr constop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		constop.reset(new VM::Operations::PushIntegerLiteral(1));		break;
	case VM::EpochVariableType_Integer16:	constop.reset(new VM::Operations::PushInteger16Literal(1));		break;
	case VM::EpochVariableType_Real:		constop.reset(new VM::Operations::PushRealLiteral(1.0f));		break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}
	AddOperationDeferred(constop);

	VM::OperationPtr innerop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Integer:		innerop.reset(new VM::Operations::SubtractIntegers(false, false));		break;
	case VM::EpochVariableType_Integer16:	innerop.reset(new VM::Operations::SubtractInteger16s(false, false));	break;
	case VM::EpochVariableType_Real:		innerop.reset(new VM::Operations::SubtractReals(false, false));			break;
	default:								throw ParserFailureException("Invalid variable type for this operator");
	}

	AddOperationDeferred(VM::OperationPtr(new VM::Operations::PushOperation(innerop.release())));
	AddOperationDeferred(VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec]))));
}


//
// Inject operation needed to provide an inline increment or decrement
//
void ParserState::HandleInlineIncDec()
{
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushOperation(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(SavedStringSlots[SavedStringSlot_IncDec])))));

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_OPERATION;
	entry.OperationPointer = Blocks.back().TheBlock->GetTailOperation();
	TheStack.push_back(entry);

	CountParameter();
}


//
// Helper structure used to automatically set up infix operand data on startup
//
namespace 
{

	struct autoinit
	{

		autoinit()
		{
			DefineInfixOperator(Operators::Add, Keywords::Add, OPREC_ADDITION);
			DefineInfixOperator(Operators::Subtract, Keywords::Subtract, OPREC_ADDITION);
			DefineInfixOperator(Operators::Multiply, Keywords::Multiply, OPREC_MULTIPLICATION);
			DefineInfixOperator(Operators::Divide, Keywords::Divide, OPREC_MULTIPLICATION);

			DefineInfixOperator(Operators::AddAssign, Keywords::Add, OPREC_CALCASSIGN);
			DefineInfixOperator(Operators::SubtractAssign, Keywords::Subtract, OPREC_CALCASSIGN);
			DefineInfixOperator(Operators::MultiplyAssign, Keywords::Multiply, OPREC_CALCASSIGN);
			DefineInfixOperator(Operators::DivideAssign, Keywords::Divide, OPREC_CALCASSIGN);

			DefineInfixOperator(Operators::Increment, Keywords::Add, OPREC_INCREMENT);
			DefineInfixOperator(Operators::Decrement, Keywords::Subtract, OPREC_INCREMENT);

			DefineInfixOperator(Operators::Greater, Keywords::Greater, OPREC_COMPARISON);
			DefineInfixOperator(Operators::GreaterEqual, Keywords::GreaterEqual, OPREC_COMPARISON);
			DefineInfixOperator(Operators::Less, Keywords::Less, OPREC_COMPARISON);
			DefineInfixOperator(Operators::LessEqual, Keywords::LessEqual, OPREC_COMPARISON);
			DefineInfixOperator(Operators::Equal, Keywords::Equal, OPREC_EQUALITY);
			DefineInfixOperator(Operators::NotEqual, Keywords::NotEqual, OPREC_EQUALITY);

			DefineInfixOperator(Operators::And, Keywords::And, OPREC_BOOLEAN);
			DefineInfixOperator(Operators::Or, Keywords::Or, OPREC_BOOLEAN);
			DefineInfixOperator(Operators::Xor, Keywords::Xor, OPREC_BOOLEAN);

			DefineInfixOperator(Operators::Concat, Keywords::Concat, OPREC_CONCATENATION);
			DefineInfixOperator(Operators::ConcatAssign, Keywords::Concat, OPREC_CALCASSIGN);

			DefineInfixOperator(Operators::Assign, Keywords::Assign, OPREC_ASSIGNMENT);
		}

	} initinfix;

}

