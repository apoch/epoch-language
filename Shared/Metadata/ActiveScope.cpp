//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for containing a lexical scope and its contents
//

#include "pch.h"

#include "Metadata/ActiveScope.h"
#include "Metadata/ScopeDescription.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Virtual Machine/TypeInfo.h"

#include "Utility/Memory/Stack.h"



void ActiveScope::BindParametersToStack(const VM::ExecutionContext& context)
{
	char* stackpointer = reinterpret_cast<char*>(context.State.Stack.GetCurrentTopOfStack());

	for(std::vector<ScopeDescription::VariableEntry>::const_reverse_iterator iter = OriginalScope.Variables.rbegin(); iter != OriginalScope.Variables.rend(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
		{
			VariableStorageLocations[context.OwnerVM.GetPooledStringHandle(iter->Identifier)] = stackpointer;
			stackpointer += VM::GetStorageSize(iter->Type);
		}
	}
}

void ActiveScope::PushLocalsOntoStack(VM::ExecutionContext& context)
{
	size_t usedspace = 0;

	for(std::vector<ScopeDescription::VariableEntry>::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_LOCAL)
		{
			VariableStorageLocations[context.OwnerVM.GetPooledStringHandle(iter->Identifier)] = context.State.Stack.GetCurrentTopOfStack();

			size_t size = VM::GetStorageSize(iter->Type);
			context.State.Stack.Push(size);
			usedspace += size;
		}
	}
}

void ActiveScope::PopScopeOffStack(VM::ExecutionContext& context)
{
	size_t usedspace = 0;

	for(std::vector<ScopeDescription::VariableEntry>::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
		usedspace += VM::GetStorageSize(iter->Type);

	context.State.Stack.Pop(usedspace);
}


void ActiveScope::Write(StringHandle variableid, Integer32 value)
{
	std::map<StringHandle, void*>::const_iterator iter = VariableStorageLocations.find(variableid);
	if(iter == VariableStorageLocations.end())
		throw std::exception("Requested variable has not been bound to any storage");

	*reinterpret_cast<Integer32*>(iter->second) = value;
}


void ActiveScope::PushOntoStack(StringHandle variableid, StackSpace& stack) const
{
	switch(OriginalScope.GetVariableTypeByID(variableid))
	{
	case VM::EpochType_Integer:
		{
			Integer32* value = reinterpret_cast<Integer32*>(GetVariableStorageLocation(variableid));
			stack.PushValue(*value);
		}
		break;

	case VM::EpochType_String:
		{
			StringHandle* value = reinterpret_cast<StringHandle*>(GetVariableStorageLocation(variableid));
			stack.PushValue(*value);
		}
		break;

	default:
		throw std::exception("Not implemented");
	}
}


void* ActiveScope::GetVariableStorageLocation(StringHandle variableid) const
{
	std::map<StringHandle, void*>::const_iterator iter = VariableStorageLocations.find(variableid);
	if(iter == VariableStorageLocations.end())
		throw std::exception("Invalid variable ID");

	return iter->second;
}