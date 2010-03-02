//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - map and reduce functions
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"
#include "Parser/Error Handling/ParserExceptions.h"

#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/Containers/MapReduce.h"
#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Operations/Variables/StringOps.h"
#include "Virtual Machine/Operations/Flow/Invoke.h"
#include "Virtual Machine/Operations/Debugging.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Function.h"

#include "Virtual Machine/VMExceptions.h"


using namespace Parser;

using namespace VM;
using namespace VM::Operations;


//
// Create an operation that invokes the map function.
//
// Map applies a given unary function to each entry in the container,
// and returns a container containing the results of the operation.
//
VM::OperationPtr ParserState::CreateOperation_Map()
{
	StackEntry p2 = TheStack.back();
	if(p2.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Expected a function identifier");
		TheStack.pop_back();
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	
	TheStack.pop_back();
	StackEntry p1 = TheStack.back();
	TheStack.pop_back();

	if(p1.Type == StackEntry::STACKENTRYTYPE_OPERATION)
	{
		if(!dynamic_cast<VM::Operations::ConsArray*>(p1.OperationPointer) && p1.OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_Array)
		{
			ReportFatalError("First parameter to map() must be an array");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}
	else if(p1.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Expected name of an array");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	VM::OperationPtr op(NULL);

	VM::EpochVariableTypeID elementtype;

	if(p1.Type == StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		elementtype = ArrayTypes[p1.StringValue];
	}
	else
	{
		elementtype = p1.OperationPointer->GetType(*CurrentScope);
		VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(p1.OperationPointer);
		if(consop)
			elementtype = consop->GetElementType();
	}

	if(p2.StringValue == Keywords::DebugWrite)
	{
		if(elementtype != VM::EpochVariableType_String)
		{
			ReportFatalError("debugwritestring() expects a string parameter; this array does not contain strings");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		op.reset(new VM::Operations::DebugWriteStringExpression);
	}
	else
	{
		VM::FunctionBase* func;
		try
		{
			func = CurrentScope->GetFunction(p2.StringValue);
		}
		catch(VM::ExecutionException&)
		{
			ReportFatalError("This function cannot be applied via map(), or no function by that name exists");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		size_t paramcount = func->GetParams().GetMemberOrder().size();
		if(paramcount != 1)
		{
			ReportFatalError("map() must be passed a function of 1 parameter");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(elementtype == VM::EpochVariableType_Array)
		{
			// Walk the code backwards to locate the corresponding array cons operation
			const std::vector<Operation*>& ops = Blocks.back().TheBlock->GetAllOperations();
			bool foundtype = false;
			for(std::vector<Operation*>::const_reverse_iterator iter = ops.rbegin(); iter != ops.rend(); ++iter)
			{
				VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(*iter);
				if(!consop)
				{
					VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(*iter);
					if(!pushop)
						throw ParserFailureException("Failed to locate array cons");

					consop = dynamic_cast<VM::Operations::ConsArray*>(pushop->GetNestedOperation());
					if(consop)
					{
						elementtype = consop->GetElementType();
						foundtype = true;
						break;
					}
				}
			}

			if(!foundtype)
				throw ParserFailureException("Failed to determine array element type");
		}

		VM::EpochVariableTypeID paramtype = func->GetParams().GetVariableType(0);
		if(paramtype != elementtype)
		{
			ReportFatalError("This function cannot be used to operate on an array of this type");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		op.reset(new VM::Operations::Invoke(func, false));
	}

	return VM::OperationPtr(new VM::Operations::MapOperation(op));
}


//
// Create an operation that invokes the reduce function.
//
// Reduce combines each entry in the container using a given binary
// function. The function is applied to each value in the container,
// and the running "result" variable. The result is a single
// value representing the result at the end of the reduction
// operation.
//
VM::OperationPtr ParserState::CreateOperation_Reduce()
{
	StackEntry p2 = TheStack.back();
	if(p2.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Expected a function identifier");
		TheStack.pop_back();
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	
	TheStack.pop_back();
	StackEntry p1 = TheStack.back();
	TheStack.pop_back();

	if(p1.Type == StackEntry::STACKENTRYTYPE_OPERATION)
	{
		VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(p1.OperationPointer);
		if(!consop && p1.OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_Array)
		{
			ReportFatalError("First parameter to reduce() must be an array");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}
	else if(p1.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Expected name of an array");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	VM::OperationPtr op(NULL);

	VM::EpochVariableTypeID elementtype;

	if(p1.Type == StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		elementtype = ArrayTypes[p1.StringValue];
	}
	else
	{
		elementtype = p1.OperationPointer->GetType(*CurrentScope);
		VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(p1.OperationPointer);
		if(consop)
			elementtype = consop->GetElementType();
	}

	if(p2.StringValue == Keywords::Add)
	{
		switch(elementtype)
		{
		case VM::EpochVariableType_Integer:		op.reset(new VM::Operations::SumIntegers(false, false));			break;
		case VM::EpochVariableType_Integer16:	op.reset(new VM::Operations::SumInteger16s(false, false));			break;
		case VM::EpochVariableType_Real:		op.reset(new VM::Operations::SumReals(false, false));				break;
		default:
			ReportFatalError("Cannot add() parameters of this type");
			return VM::OperationPtr(new VM::Operations::NoOp);			
		}
	}
	else if(p2.StringValue == Keywords::Subtract)
	{
		switch(elementtype)
		{
		case VM::EpochVariableType_Integer:		op.reset(new VM::Operations::SubtractIntegers(false, false));		break;
		case VM::EpochVariableType_Integer16:	op.reset(new VM::Operations::SubtractInteger16s(false, false));		break;
		case VM::EpochVariableType_Real:		op.reset(new VM::Operations::SubtractReals(false, false));			break;
		default:
			ReportFatalError("Cannot subtract() parameters of this type");
			return VM::OperationPtr(new VM::Operations::NoOp);			
		}
	}
	else if(p2.StringValue == Keywords::Multiply)
	{
		switch(elementtype)
		{
		case VM::EpochVariableType_Integer:		op.reset(new VM::Operations::MultiplyIntegers(false, false));		break;
		case VM::EpochVariableType_Integer16:	op.reset(new VM::Operations::MultiplyInteger16s(false, false));		break;
		case VM::EpochVariableType_Real:		op.reset(new VM::Operations::MultiplyReals(false, false));			break;
		default:
			ReportFatalError("Cannot multiply() parameters of this type");
			return VM::OperationPtr(new VM::Operations::NoOp);			
		}
	}
	else if(p2.StringValue == Keywords::Divide)
	{
		switch(elementtype)
		{
		case VM::EpochVariableType_Integer:		op.reset(new VM::Operations::DivideIntegers(false, false));			break;
		case VM::EpochVariableType_Integer16:	op.reset(new VM::Operations::DivideInteger16s(false, false));		break;
		case VM::EpochVariableType_Real:		op.reset(new VM::Operations::DivideReals(false, false));			break;
		default:
			ReportFatalError("Cannot divide() parameters of this type");
			return VM::OperationPtr(new VM::Operations::NoOp);			
		}
	}
	else if(p2.StringValue == Keywords::Concat)
	{
		if(elementtype != VM::EpochVariableType_String)
		{
			ReportFatalError("concat() requires an array of strings");
			return VM::OperationPtr(new VM::Operations::NoOp);	
		}

		op.reset(new VM::Operations::Concatenate(false, false));
	}
	else
	{
		VM::FunctionBase* func;
		try
		{
			func = CurrentScope->GetFunction(p2.StringValue);
		}
		catch(ExecutionException&)
		{
			ReportFatalError("This function cannot be applied via reduce(), or no function by that name exists");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		size_t paramcount = func->GetParams().GetMemberOrder().size();
		if(paramcount != 2)
		{
			ReportFatalError("reduce() must be passed a function of 2 parameters");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(func->GetParams().GetVariableType(0) != elementtype || func->GetParams().GetVariableType(1) != elementtype)
		{
			ReportFatalError("Cannot reduce() using this function - parameter types are incorrect");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		op.reset(new VM::Operations::Invoke(func, false));
	}

	return VM::OperationPtr(new VM::Operations::ReduceOperation(op));
}


