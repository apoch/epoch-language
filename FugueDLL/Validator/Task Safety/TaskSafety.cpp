//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Validation logic for ensuring that asynchronous tasks are safe
//

#include "pch.h"

#include "Virtual Machine/Operations/Concurrency/FutureOps.h"
#include "Virtual Machine/Operations/Concurrency/Messaging.h"
#include "Virtual Machine/Operations/Concurrency/Tasks.h"
#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/Containers/MapReduce.h"
#include "Virtual Machine/Operations/Flow/FlowControl.h"
#include "Virtual Machine/Operations/Flow/Invoke.h"
#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Operations/Operators/Bitwise.h"
#include "Virtual Machine/Operations/Operators/Comparison.h"
#include "Virtual Machine/Operations/Operators/CompoundOperator.h"
#include "Virtual Machine/Operations/Operators/Logical.h"
#include "Virtual Machine/Operations/Variables/StringOps.h"
#include "Virtual Machine/Operations/Variables/StructureOps.h"
#include "Virtual Machine/Operations/Variables/TupleOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Debugging.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"
#include "Virtual Machine/Operations/Typedefs.h"

#include "Marshalling/ExternalDLL.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Program.h"

#include "Virtual Machine/Types Management/RuntimeCasts.h"
#include "Virtual Machine/Types Management/TypeInfo.h"

#include "Language Extensions/Handoff.h"

#include "Validator/Task Safety/TaskSafety.h"
#include "Validator/Validator.h"



using namespace Validator;


//
// If the given operation is invoked from within a task, signal a validation failure
//
void TaskSafetyWrapper::OpInvalidInTask(ValidationTraverser& traverser, const VM::Operation* op)
{
	if(traverser.HasAlreadySeenOp(op))
		return;

	if(traverser.IsInTask())
	{
		traverser.FlagError(op, L"This operation cannot be performed within a task");
		traverser.Valid = false;
	}

	traverser.RecordTraversedOp(op);
}

//
// If the given operation accesses non-constant global data from within a task, signal a validation failure
//
void TaskSafetyWrapper::OpMustNotAccessGlobalState(ValidationTraverser& traverser, const std::wstring& varname, const VM::Operation* op)
{
	if(traverser.HasAlreadySeenOp(op))
		return;

	if(traverser.IsInTask())
	{
		if(!traverser.CurrentScope)
			throw VM::InternalFailureException("Tried to validate a variable access operation, but no variable scope is currently set");

		const VM::ScopeDescription* ownerscope = traverser.CurrentScope->GetScopeOwningVariable(varname);
		if(ownerscope == &(traverser.CurrentProgram->GetGlobalScope()))
		{
			if(!ownerscope->IsConstant(varname))
			{
				traverser.FlagError(op, L"Global variables may not be accessed from within a task");
				traverser.Valid = false;
			}
		}
	}

	traverser.RecordTraversedOp(op);
}


#define VALIDATOR_TEMPLATE(operationname) \
	template <> void Validator::TaskSafetyCheck<operationname>(const operationname& op, ValidationTraverser& traverser)


#define VALIDATE_ALWAYS_VALID(operationname) \
	VALIDATOR_TEMPLATE(operationname) { }

#define VALIDATE_NOT_IN_TASKS(operationname) \
	VALIDATOR_TEMPLATE(operationname) { TaskSafetyWrapper::OpInvalidInTask(traverser, &op); }

#define VALIDATE_ONLY_CONST_GLOBALS(operationname) \
	VALIDATOR_TEMPLATE(operationname) { TaskSafetyWrapper::OpMustNotAccessGlobalState(traverser, op.GetAssociatedIdentifier(), &op); }



// Operations which are always valid
VALIDATE_ALWAYS_VALID(VM::Operations::AcceptMessage)
VALIDATE_ALWAYS_VALID(VM::Operations::AcceptMessageFromResponseMap)
VALIDATE_ALWAYS_VALID(VM::Operations::AssignStructureIndirect)
VALIDATE_ALWAYS_VALID(VM::Operations::BitwiseAnd)
VALIDATE_ALWAYS_VALID(VM::Operations::BitwiseNot)
VALIDATE_ALWAYS_VALID(VM::Operations::BitwiseOr)
VALIDATE_ALWAYS_VALID(VM::Operations::BitwiseXor)
VALIDATE_ALWAYS_VALID(VM::Operations::BooleanConstant)
VALIDATE_ALWAYS_VALID(VM::Operations::Break)
VALIDATE_ALWAYS_VALID(VM::Operations::Concatenate)
VALIDATE_ALWAYS_VALID(VM::Operations::ConsList)
VALIDATE_ALWAYS_VALID(VM::Operations::CreateThreadPool)
VALIDATE_ALWAYS_VALID(VM::Operations::DebugCrashVM)
VALIDATE_ALWAYS_VALID(VM::Operations::DivideInteger16s)
VALIDATE_ALWAYS_VALID(VM::Operations::DivideIntegers)
VALIDATE_ALWAYS_VALID(VM::Operations::DivideReals)
VALIDATE_ALWAYS_VALID(VM::Operations::DoWhileLoop)
VALIDATE_ALWAYS_VALID(VM::Operations::ElseIf)
VALIDATE_ALWAYS_VALID(VM::Operations::ElseIfWrapper)
VALIDATE_ALWAYS_VALID(VM::Operations::ExecuteBlock)
VALIDATE_ALWAYS_VALID(VM::Operations::ExitIfChain)
VALIDATE_ALWAYS_VALID(VM::Operations::ForkFuture)
VALIDATE_ALWAYS_VALID(VM::Operations::ForkTask)
VALIDATE_ALWAYS_VALID(VM::Operations::ForkThread)
VALIDATE_ALWAYS_VALID(VM::Operations::GetMessageSender)
VALIDATE_ALWAYS_VALID(VM::Operations::GetTaskCaller)
VALIDATE_ALWAYS_VALID(VM::Operations::If)
VALIDATE_ALWAYS_VALID(VM::Operations::IntegerConstant)
VALIDATE_ALWAYS_VALID(VM::Operations::Integer16Constant)
VALIDATE_ALWAYS_VALID(VM::Operations::Invoke)
VALIDATE_ALWAYS_VALID(VM::Operations::InvokeIndirect)
VALIDATE_ALWAYS_VALID(VM::Operations::IsEqual)
VALIDATE_ALWAYS_VALID(VM::Operations::IsGreater)
VALIDATE_ALWAYS_VALID(VM::Operations::IsGreaterOrEqual)
VALIDATE_ALWAYS_VALID(VM::Operations::IsLesser)
VALIDATE_ALWAYS_VALID(VM::Operations::IsLesserOrEqual)
VALIDATE_ALWAYS_VALID(VM::Operations::IsNotEqual)
VALIDATE_ALWAYS_VALID(VM::Operations::LogicalAnd)
VALIDATE_ALWAYS_VALID(VM::Operations::LogicalNot)
VALIDATE_ALWAYS_VALID(VM::Operations::LogicalOr)
VALIDATE_ALWAYS_VALID(VM::Operations::LogicalXor)
VALIDATE_ALWAYS_VALID(VM::Operations::MapOperation)
VALIDATE_ALWAYS_VALID(VM::Operations::MultiplyInteger16s)
VALIDATE_ALWAYS_VALID(VM::Operations::MultiplyIntegers)
VALIDATE_ALWAYS_VALID(VM::Operations::MultiplyReals)
VALIDATE_ALWAYS_VALID(VM::Operations::Negate)
VALIDATE_ALWAYS_VALID(VM::Operations::NoOp)
VALIDATE_ALWAYS_VALID(VM::Operations::PushBooleanLiteral)
VALIDATE_ALWAYS_VALID(VM::Operations::PushInteger16Literal)
VALIDATE_ALWAYS_VALID(VM::Operations::PushIntegerLiteral)
VALIDATE_ALWAYS_VALID(VM::Operations::PushOperation)
VALIDATE_ALWAYS_VALID(VM::Operations::PushRealLiteral)
VALIDATE_ALWAYS_VALID(VM::Operations::PushStringLiteral)
VALIDATE_ALWAYS_VALID(VM::Operations::RealConstant)
VALIDATE_ALWAYS_VALID(VM::Operations::ReadStructureIndirect)
VALIDATE_ALWAYS_VALID(VM::Operations::ReduceOperation)
VALIDATE_ALWAYS_VALID(VM::Operations::Return)
VALIDATE_ALWAYS_VALID(VM::Operations::SendTaskMessage)
VALIDATE_ALWAYS_VALID(VM::Operations::SubtractInteger16s)
VALIDATE_ALWAYS_VALID(VM::Operations::SubtractIntegers)
VALIDATE_ALWAYS_VALID(VM::Operations::SubtractReals)
VALIDATE_ALWAYS_VALID(VM::Operations::SumInteger16s)
VALIDATE_ALWAYS_VALID(VM::Operations::SumIntegers)
VALIDATE_ALWAYS_VALID(VM::Operations::SumReals)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastBooleanToString)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastBufferToString)
VALIDATE_ALWAYS_VALID(VM::Operations::WhileLoop)
VALIDATE_ALWAYS_VALID(VM::Operations::WhileLoopConditional)

VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastStringToInteger)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastRealToInteger)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastInteger16ToInteger)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastBooleanToInteger)

VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastStringToInteger16)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastRealToInteger16)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastIntegerToInteger16)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastBooleanToInteger16)

VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastStringToReal)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastInteger16ToReal)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastIntegerToReal)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastBooleanToReal)

VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastRealToString)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastInteger16ToString)
VALIDATE_ALWAYS_VALID(VM::Operations::TypeCastIntegerToString)

VALIDATE_ALWAYS_VALID(Extensions::HandoffOperation)
VALIDATE_ALWAYS_VALID(Marshalling::CallDLL)


// Operations which are not permitted within tasks
VALIDATE_NOT_IN_TASKS(VM::Operations::DebugReadStaticString)
VALIDATE_NOT_IN_TASKS(VM::Operations::DebugWriteStringExpression)


// Operations which cannot access global mutable state from within tasks
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::AssignStructure)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::AssignTuple)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::AssignValue)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::BindFunctionReference)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::BindReference)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::BindStructMemberReference)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::GetVariableValue)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::InitializeValue)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::Length)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::ReadStructure)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::ReadTuple)
VALIDATE_ONLY_CONST_GLOBALS(VM::Operations::SizeOf)


// Additional validation traversal logic
template <> void Validator::TaskSafetyCheck<VM::Function>(const VM::Function& func, ValidationTraverser& traverser) { }

