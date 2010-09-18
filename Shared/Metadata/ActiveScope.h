//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for containing a lexical scope and its contents
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/IDTypes.h"

#include <map>


// Forward declarations
class StackSpace;
class ScopeDescription;
class Register;
namespace VM { class ExecutionContext; }


class ActiveScope
{
// Construction
public:
	ActiveScope(const ScopeDescription& originalscope, ActiveScope* parent)
		: OriginalScope(originalscope),
		  ParentScope(parent)
	{ }

// Interface for attaching scope to actual memory storage
public:
	void BindParametersToStack(const VM::ExecutionContext& context);

	void PushLocalsOntoStack(VM::ExecutionContext& context);

	void PopScopeOffStack(VM::ExecutionContext& context);

// Variable manipulation interface
public:
	template <typename T>
	void Write(StringHandle variableid, T value)
	{
		std::map<StringHandle, void*>::const_iterator iter = VariableStorageLocations.find(variableid);
		if(iter == VariableStorageLocations.end())
			throw std::exception("Requested variable has not been bound to any storage");

		*reinterpret_cast<T*>(iter->second) = value;
	}

	void WriteFromStack(StringHandle variableid, StackSpace& stack);

	void PushOntoStack(StringHandle variableid, StackSpace& stack) const;

// Interaction with registers
public:
	void CopyToRegister(StringHandle variableid, Register& targetregister) const;

// State queries
public:
	bool HasReturnVariable() const;

// Access to parent scope
public:
	ActiveScope* ParentScope;

// Internal helpers
private:
	void* GetVariableStorageLocation(StringHandle variableid) const;

// Internal tracking
private:
	const ScopeDescription& OriginalScope;

	std::map<StringHandle, void*> VariableStorageLocations;
};

