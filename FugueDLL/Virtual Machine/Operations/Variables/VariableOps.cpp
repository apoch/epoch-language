//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with variables and their values
//

#include "pch.h"

#include "Virtual Machine/Operations/Variables/VariableOps.h"

#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"
#include "Virtual Machine/Core Entities/Variables/ListVariable.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/SelfAware.inl"

#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/Types Management/Typecasts.h"


using namespace VM;
using namespace VM::Operations;


//
// Construct a variable assignment operation
//
AssignValue::AssignValue(const std::wstring& varname)
	: VarName(varname)
{
}

//
// Assign an r-value into a variable (l-value)
//
RValuePtr AssignValue::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	return scope.PopVariableOffStack(VarName, stack, false);
}

void AssignValue::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	scope.PopVariableOffStack(VarName, stack, false);
}


//
// Retrieve the type of the assignment operation
// (same as type of variable being assigned)
//
EpochVariableTypeID AssignValue::GetType(const ScopeDescription& scope) const
{
	return scope.GetVariableType(VarName);
}

size_t AssignValue::GetNumParameters(const VM::ScopeDescription& scope) const
{
	EpochVariableTypeID type = GetType(scope);
	if(type == EpochVariableType_Structure || type == EpochVariableType_Tuple)
		return 2;
	else if(type == EpochVariableType_List)
		return 3;

	return 1;
}

Traverser::Payload AssignValue::GetNodeTraversalPayload() const
{
	Traverser::Payload ret;
	ret.SetValue(VarName.c_str());
	ret.IsIdentifier = true;
	return ret;
}



//
// Construct a variable intialization operation
//
InitializeValue::InitializeValue(const std::wstring& varname)
	: VarName(varname)
{
}

//
// Assign an r-value into a variable (l-value)
//
RValuePtr InitializeValue::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	if(scope.GetVariableType(VarName) == EpochVariableType_Buffer)
	{
		IntegerVariable var(stack.GetCurrentTopOfStack());
		size_t buffersize = var.GetValue();
		stack.Pop(IntegerVariable::GetStorageSize());

		scope.GetVariableRef(VarName).CastTo<BufferVariable>().SetValue(NULL, buffersize, true);
		return scope.GetVariableValue(VarName);
	}
	else if(scope.GetVariableType(VarName) == EpochVariableType_List)
	{
		ListVariable& thelist = scope.GetVariableRef<ListVariable>(VarName);
		thelist.BindToStorage(stack.GetCurrentTopOfStack());
		return thelist.GetAsRValue();
	}
	else
		return scope.PopVariableOffStack(VarName, stack, true);
}

void InitializeValue::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}


//
// Retrieve the type of the assignment operation
// (same as type of variable being assigned)
//
EpochVariableTypeID InitializeValue::GetType(const ScopeDescription& scope) const
{
	return scope.GetVariableType(VarName);
}

size_t InitializeValue::GetNumParameters(const VM::ScopeDescription& scope) const
{
	EpochVariableTypeID type = GetType(scope);
	if(type == EpochVariableType_Structure || type == EpochVariableType_Tuple)
		return 2;
	else if(type == EpochVariableType_List)
		return 3;

	return 1;
}

Traverser::Payload InitializeValue::GetNodeTraversalPayload() const
{
	Traverser::Payload ret;
	ret.SetValue(VarName.c_str());
	ret.IsIdentifier = true;
	return ret;
}


//
// Construct an operation which retrieves a variable's value
//
GetVariableValue::GetVariableValue(const std::wstring& varname)
	: VarName(varname)
{
}

//
// Retrieve a variable's value from the given scope
//
RValuePtr GetVariableValue::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	return scope.GetVariableValue(VarName);
}

void GetVariableValue::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	// Nothing to do.
}

//
// Get the type of the retrieved variable
//
EpochVariableTypeID GetVariableValue::GetType(const ScopeDescription& scope) const
{
	return scope.GetVariableType(VarName);
}

Traverser::Payload GetVariableValue::GetNodeTraversalPayload() const
{
	Traverser::Payload ret;
	ret.SetValue(VarName.c_str());
	ret.IsIdentifier = true;
	return ret;
}


//
// Retrieve a variable's storage size
//
RValuePtr SizeOf::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	if(scope.HasStructureType(VarName))
		return RValuePtr(new IntegerRValue(static_cast<Integer32>(scope.GetStructureType(VarName).GetMemberStorageSize())));
	else if(scope.HasTupleType(VarName))
		return RValuePtr(new IntegerRValue(static_cast<Integer32>(scope.GetTupleType(VarName).GetTotalSize())));
	else if(scope.GetVariableType(VarName) == EpochVariableType_Buffer)
		return RValuePtr(new IntegerRValue(static_cast<Integer32>(scope.GetVariableRef<BufferVariable>(VarName).GetSize())));

	return RValuePtr(new IntegerRValue(static_cast<Integer32>(TypeInfo::GetStorageSize(scope.GetVariableType(VarName)))));
}

void SizeOf::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	// Nothing to do.
}

Traverser::Payload SizeOf::GetNodeTraversalPayload() const
{
	Traverser::Payload payload;
	payload.SetValue(VarName.c_str());
	payload.IsIdentifier = true;
	return payload;
}