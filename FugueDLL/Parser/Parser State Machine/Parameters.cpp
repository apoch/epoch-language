//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parameter passing manager routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"
#include "Parser/Tracing.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/SelfAware.inl"

#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Operators/Logical.h"
#include "Virtual Machine/Operations/Operators/Bitwise.h"
#include "Virtual Machine/Operations/Operators/Arithmetic.h"

#include "Virtual Machine/VMExceptions.h"

#include "Virtual Machine/Types Management/TypeInfo.h"


using namespace Parser;


//
// Start counting the number of parameters passed to the current operation
//
void ParserState::StartCountingParams()
{
	PassedParameterCount.push(0);
	InfixOperandCount.push_back(0);
	InfixOperatorList.push(std::list<std::wstring>());
}

//
// Count a parameter that was not otherwise counted (usually identifiers)
//
void ParserState::CountParameter()
{
	++PassedParameterCount.top();
}

//
// Register that an operation should be called, and its return value
// pushed onto the stack as a parameter to a separate function call.
//
void ParserState::PushOperationAsParameter(const std::wstring& operationname)
{
	if(operationname.empty())		// Empty opname signals a parenthetical expression, no operation required
		return;

	VM::OperationPtr innerop(CreateOperation(operationname));

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_OPERATION;
	entry.OperationPointer = innerop.get();
	TheStack.push_back(entry);

	VM::EpochVariableTypeID type = innerop->GetType(*CurrentScope);


	try
	{
		VM::OperationPtr pushop(new VM::Operations::PushOperation(innerop.release(), *CurrentScope));
		AddOperationToCurrentBlock(pushop);
	}
	catch(VM::MissingVariableException& e)
	{
		ReportFatalError(e.what());
	}


	if(InjectNotOperator)
	{
		if(type == VM::EpochVariableType_Boolean)
		{
			VM::OperationPtr injectop(new VM::Operations::PushOperation(new VM::Operations::LogicalNot, *CurrentScope));
			AddOperationToCurrentBlock(injectop);
		}
		else
		{
			VM::OperationPtr injectop(new VM::Operations::PushOperation(new VM::Operations::BitwiseNot(type), *CurrentScope));
			AddOperationToCurrentBlock(injectop);
		}
	}

	if(InjectNegateOperator)
	{
		if(TypeInfo::IsNumeric(type))
		{
			VM::OperationPtr injectop(new VM::Operations::PushOperation(new VM::Operations::Negate(type), *CurrentScope));
			AddOperationToCurrentBlock(injectop);
		}
		else
			ReportFatalError("Cannot negate a value of this type");
	}

	InjectNotOperator = false;
	InjectNegateOperator = false;


	if(PassedParameterCount.top() == 0)
	{
		InfixOperatorList.pop();
		InfixOperandCount.pop_back();
	}

	PopParameterCount();
	++PassedParameterCount.top();
}

//
// Register a variable or function identifier; what is done with it
// depends on other contextual information from the analyzer.
//
void ParserState::PushIdentifier(const std::wstring& identifier)
{
	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_IDENTIFIER;
	entry.StringValue = identifier;
	TheStack.push_back(entry);
}

//
// Register a variable that should be passed on the stack to a function call.
//
void ParserState::PushIdentifierAsParameter(const std::wstring& identifier)
{
	VM::OperationPtr op(new VM::Operations::PushOperation(new VM::Operations::GetVariableValue(ParsedProgram->PoolStaticString(identifier)), *CurrentScope));
	AddOperationToCurrentBlock(op);

	bool leaveoponstack = false;

	if(InjectNotOperator)
	{
		leaveoponstack = true;
		VM::EpochVariableTypeID type = Blocks.back().TheBlock->GetTailOperation()->GetType(*CurrentScope);
		if(type == VM::EpochVariableType_Boolean)
		{
			VM::OperationPtr injectop(new VM::Operations::PushOperation(new VM::Operations::LogicalNot, *CurrentScope));
			AddOperationToCurrentBlock(injectop);
		}
		else
		{
			VM::OperationPtr injectop(new VM::Operations::PushOperation(new VM::Operations::BitwiseNot(type), *CurrentScope));
			AddOperationToCurrentBlock(injectop);
		}
	}

	if(InjectNegateOperator)
	{
		leaveoponstack = true;
		VM::EpochVariableTypeID type = Blocks.back().TheBlock->GetTailOperation()->GetType(*CurrentScope);
		if(TypeInfo::IsNumeric(type))
		{
			VM::OperationPtr injectop(new VM::Operations::PushOperation(new VM::Operations::Negate(type), *CurrentScope));
			AddOperationToCurrentBlock(injectop);
		}
		else
			ReportFatalError("Cannot negate a value of this type");
	}

	InjectNotOperator = false;
	InjectNegateOperator = false;

	if(leaveoponstack)
	{
		StackEntry entry;
		entry.Type = StackEntry::STACKENTRYTYPE_OPERATION;
		entry.OperationPointer = Blocks.back().TheBlock->GetTailOperation();
		TheStack.push_back(entry);
	}
	else
	{
		StackEntry entry;
		entry.Type = StackEntry::STACKENTRYTYPE_IDENTIFIER;
		entry.StringValue = identifier;
		TheStack.push_back(entry);
	}

	++PassedParameterCount.top();
}

//
// Register an integer literal that should be passed on the stack to a function call.
//
void ParserState::PushIntegerLiteral(Integer32 value)
{
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(value)));

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_INTEGER_LITERAL;
	entry.IntegerValue = value;
	TheStack.push_back(entry);

	++PassedParameterCount.top();
}

//
// Register a real literal that should be passed on the stack to a function call.
//
void ParserState::PushRealLiteral(Real value)
{
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushRealLiteral(value)));

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_REAL_LITERAL;
	entry.RealValue = value;
	TheStack.push_back(entry);

	++PassedParameterCount.top();
}

//
// Register a string literal that should be passed on the stack to a function call.
//
void ParserState::PushStringLiteral(const std::wstring& value)
{
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushStringLiteral(value)));

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_STRING_LITERAL;
	entry.StringValue = value;
	TheStack.push_back(entry);

	++PassedParameterCount.top();
}

//
// Register that a string literal is being passed, but do not evaluate it
// or place its value on the stack. This is mainly used for assigning to
// variables, passing first-class functions and types, and so on.
//
void ParserState::PushStringLiteralNoStack(const std::wstring& value)
{
	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_STRING_LITERAL;
	entry.StringValue = value;
	TheStack.push_back(entry);

	++PassedParameterCount.top();
}

//
// Register a boolean literal that should be passed on the stack to a function call.
//
void ParserState::PushBooleanLiteral(bool value)
{
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushBooleanLiteral(value)));

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_BOOLEAN_LITERAL;
	entry.BooleanValue = value;
	TheStack.push_back(entry);

	++PassedParameterCount.top();
}


void ParserState::PopParameterCount()
{
	Trace(L"Reset passed parameter count");
	PassedParameterCount.pop();
}

