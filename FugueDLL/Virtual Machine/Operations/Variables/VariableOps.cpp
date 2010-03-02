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
#include "Virtual Machine/Core Entities/Variables/ArrayVariable.h"
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
RValuePtr AssignValue::ExecuteAndStoreRValue(ExecutionContext& context)
{
	return context.Scope.PopVariableOffStack(VarName, context.Stack, false);
}

void AssignValue::ExecuteFast(ExecutionContext& context)
{
	context.Scope.PopVariableOffStack(VarName, context.Stack, false);
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
	EpochVariableTypeID type = scope.GetVariableType(VarName);
	if(type == EpochVariableType_Structure || type == EpochVariableType_Tuple)
		return 2;
	else if(type == EpochVariableType_Array)
		return 3;

	return 1;
}

Traverser::Payload AssignValue::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload ret;
	ret.SetValue(VarName.c_str());
	ret.IsIdentifier = true;
	ret.ParameterCount = GetNumParameters(*scope);
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
RValuePtr InitializeValue::ExecuteAndStoreRValue(ExecutionContext& context)
{
	if(context.Scope.GetVariableType(VarName) == EpochVariableType_Buffer)
	{
		IntegerVariable var(context.Stack.GetCurrentTopOfStack());
		size_t buffersize = var.GetValue();
		context.Stack.Pop(IntegerVariable::GetStorageSize());

		context.Scope.GetVariableRef(VarName).CastTo<BufferVariable>().SetValue(NULL, buffersize, true);
		return context.Scope.GetVariableValue(VarName);
	}
	else if(context.Scope.GetVariableType(VarName) == EpochVariableType_Array)
	{
		IntegerVariable var(context.Stack.GetCurrentTopOfStack());
		HandleType handle = var.GetValue();
		context.Stack.Pop(IntegerVariable::GetStorageSize());

		context.Scope.GetVariableRef(VarName).CastTo<ArrayVariable>().SetValue(handle);
		return context.Scope.GetVariableValue(VarName);
	}
	else
		return context.Scope.PopVariableOffStack(VarName, context.Stack, true);
}

void InitializeValue::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
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
	EpochVariableTypeID type = scope.GetVariableType(VarName);
	if(type == EpochVariableType_Structure || type == EpochVariableType_Tuple)
		return 2;
	else if(type == EpochVariableType_Array)
		return 3;

	return 1;
}

Traverser::Payload InitializeValue::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload ret;
	ret.SetValue(VarName.c_str());
	ret.IsIdentifier = true;
	ret.ParameterCount = GetNumParameters(*scope);
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
RValuePtr GetVariableValue::ExecuteAndStoreRValue(ExecutionContext& context)
{
	return context.Scope.GetVariableValue(VarName);
}

void GetVariableValue::ExecuteFast(ExecutionContext& context)
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

Traverser::Payload GetVariableValue::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload ret;
	ret.SetValue(VarName.c_str());
	ret.IsIdentifier = true;
	ret.ParameterCount = GetNumParameters(*scope);
	return ret;
}


//
// Retrieve a variable's storage size
//
RValuePtr SizeOf::ExecuteAndStoreRValue(ExecutionContext& context)
{
	if(context.Scope.HasStructureType(VarName))
		return RValuePtr(new IntegerRValue(static_cast<Integer32>(context.Scope.GetStructureType(VarName).GetMemberStorageSize())));
	else if(context.Scope.HasTupleType(VarName))
		return RValuePtr(new IntegerRValue(static_cast<Integer32>(context.Scope.GetTupleType(VarName).GetTotalSize())));
	else if(context.Scope.GetVariableType(VarName) == EpochVariableType_Buffer)
		return RValuePtr(new IntegerRValue(static_cast<Integer32>(context.Scope.GetVariableRef<BufferVariable>(VarName).GetSize())));

	return RValuePtr(new IntegerRValue(static_cast<Integer32>(TypeInfo::GetStorageSize(context.Scope.GetVariableType(VarName)))));
}

void SizeOf::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do.
}

Traverser::Payload SizeOf::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload payload;
	payload.SetValue(VarName.c_str());
	payload.IsIdentifier = true;
	payload.ParameterCount = GetNumParameters(*scope);
	return payload;
}