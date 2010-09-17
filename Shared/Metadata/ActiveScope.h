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
	void Write(StringHandle variableid, Integer32 value);

	void PushOntoStack(StringHandle variableid, StackSpace& stack) const;

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

