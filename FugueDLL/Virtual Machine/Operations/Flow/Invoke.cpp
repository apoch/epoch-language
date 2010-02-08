//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation for invoking a function at runtime
//

#include "pch.h"

#include "Virtual Machine/Operations/Flow/Invoke.h"

#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/SelfAware.inl"


using namespace VM;
using namespace VM::Operations;


//
// Construct a function invocation operation
//
Invoke::Invoke(FunctionBase* function, bool cleanupfunction)
	: Function(function),
	  CleanUpFunction(cleanupfunction)
{
}

//
// Destruct and clean up a function invocation operation
//
Invoke::~Invoke()
{
	if(CleanUpFunction)
		delete Function;
}


//
// Invoke the bound Epoch function
//
RValuePtr Invoke::ExecuteAndStoreRValue(ExecutionContext& context)
{
	return Function->Invoke(context);
}

void Invoke::ExecuteFast(ExecutionContext& context)
{
	Function->Invoke(context);
}

//
// Retrieve the function's return type
//
EpochVariableTypeID Invoke::GetType(const ScopeDescription& scope) const
{
	return Function->GetType(scope);
}

size_t Invoke::GetNumParameters(const VM::ScopeDescription& scope) const
{
	return Function->GetParams().GetNumMembers();
}

template <typename TraverserT>
void Invoke::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	VM::Function* func = dynamic_cast<VM::Function*>(Function);
	if(func)
		func->Traverse(traverser);
}

void Invoke::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void Invoke::Traverse(Serialization::SerializationTraverser& traverser)
{
	// We do NOT want to traverse into the child when serializing, as other logic will
	// ensure that the called function is serialized in the correct location.
	traverser.TraverseNode(*this);
}

Traverser::Payload Invoke::GetNodeTraversalPayload() const
{
	Traverser::Payload payload;
	payload.SetValue(Function);
	return payload;
}

//
// Invoke the bound Epoch function
//
RValuePtr InvokeIndirect::ExecuteAndStoreRValue(ExecutionContext& context)
{
	return context.Scope.GetFunction(FunctionName)->Invoke(context);
}

void InvokeIndirect::ExecuteFast(ExecutionContext& context)
{
	context.Scope.GetFunction(FunctionName)->Invoke(context);
}

//
// Retrieve the function's return type
//
EpochVariableTypeID InvokeIndirect::GetType(const ScopeDescription& scope) const
{
	return scope.GetFunctionType(FunctionName);
}

//
// Determine how many parameters are passed to the function
//
size_t InvokeIndirect::GetNumParameters(const VM::ScopeDescription& scope) const
{
	return scope.GetFunctionSignature(FunctionName).GetParamTypes().size();
}

template <typename TraverserT>
void InvokeIndirect::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	VM::ScopeDescription* scope = traverser.GetCurrentScope();
	if(scope)
	{
		if(scope->HasFunction(FunctionName))
		{
			VM::Function* func = dynamic_cast<VM::Function*>(scope->GetFunction(FunctionName));
			if(func)
				func->Traverse(traverser);
		}
	}
}

void InvokeIndirect::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void InvokeIndirect::Traverse(Serialization::SerializationTraverser& traverser)
{
	// We do NOT want to traverse into the child when serializing, as other logic will
	// ensure that the called function is serialized in the correct location.
	traverser.TraverseNode(*this);
}

