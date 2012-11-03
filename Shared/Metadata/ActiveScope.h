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
#include "Utility/Types/EpochTypeIDs.h"

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

// Non-copyable
private:
	ActiveScope(const ActiveScope& rhs);
	ActiveScope& operator = (const ActiveScope& rhs);

// Interface for attaching scope to actual memory storage
public:
	void BindParametersToStack(const VM::ExecutionContext& context);

	void PushLocalsOntoStack(VM::ExecutionContext& context);

	void PopScopeOffStack(VM::ExecutionContext& context);

	void SetActualType(StringHandle varname, VM::EpochTypeID type);

// Variable manipulation interface
public:
	template <typename T>
	T Read(StringHandle variableid)
	{
		if(OriginalScope.IsReferenceByID(variableid))
		{
			ReferenceBindingMap::const_iterator iter = BoundReferences.find(variableid);
			if(iter == BoundReferences.end())
				throw FatalException("Unbound reference");

			return *reinterpret_cast<T*>(iter->second.first);
		}
		else
			return *reinterpret_cast<T*>(GetVariableStorageLocation(variableid));
	}

	template <typename T>
	void Write(StringHandle variableid, T value)
	{
		if(OriginalScope.IsReferenceByID(variableid))
		{
			ReferenceBindingMap::const_iterator iter = BoundReferences.find(variableid);
			if(iter == BoundReferences.end())
				throw FatalException("Unbound reference");

			Write(iter->second.first, value);
		}
		else
			*reinterpret_cast<T*>(GetVariableStorageLocation(variableid)) = value;
	}

	template <typename T>
	void Write(void* storage, T value)
	{
		*reinterpret_cast<T*>(storage) = value;
	}

	void WriteFromStack(void* targetstorage, VM::EpochTypeID targettype, StackSpace& stack);

	void PushOntoStack(void* targetstorage, VM::EpochTypeID targettype, StackSpace& stack) const;
	void PushOntoStack(StringHandle variableid, StackSpace& stack) const;
	void PushOntoStackDeref(StringHandle variableid, StackSpace& stack) const;

// References
public:
	void BindReference(StringHandle referencename, void* targetstorage, VM::EpochTypeID targettype);

	void* GetReferenceTarget(StringHandle referencename) const;
	VM::EpochTypeID GetReferenceType(StringHandle referencename) const;

// Interaction with registers
public:
	void CopyToRegister(StringHandle variableid, Register& targetregister) const;

// State queries
public:
	bool HasReturnVariable() const;

	VM::EpochTypeID GetActualType(StringHandle varname) const;

// Access to original definition metadata
public:
	const ScopeDescription& GetOriginalDescription() const
	{ return OriginalScope; }

// Access to parent scope
public:
	ActiveScope* ParentScope;

// Storage access
public:
	void* GetVariableStorageLocation(StringHandle variableid) const;

// Internal tracking
private:
	const ScopeDescription& OriginalScope;

	std::map<StringHandle, void*> VariableStorageLocations;
	
	typedef std::pair<void*, VM::EpochTypeID> ReferenceStorageAndType;
	typedef std::map<StringHandle, ReferenceStorageAndType> ReferenceBindingMap;
	ReferenceBindingMap BoundReferences;

	std::map<StringHandle, VM::EpochTypeID> ActualTypes;
};

