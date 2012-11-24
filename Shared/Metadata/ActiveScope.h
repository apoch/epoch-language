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
// Custom allocation
public:
	static void* operator new (size_t size); 
	static void operator delete (void *p);

	static void InitAllocator();

// Construction and destruction
public:
	ActiveScope(const ScopeDescription& originalscope, ActiveScope* parent);
	~ActiveScope();

// Non-copyable
private:
	ActiveScope(const ActiveScope& rhs);
	ActiveScope& operator = (const ActiveScope& rhs);

// Interface for attaching scope to actual memory storage
public:
	void BindParametersToStack(const VM::ExecutionContext& context);

	void PushLocalsOntoStack(VM::ExecutionContext& context);

	void PopScopeOffStack(VM::ExecutionContext& context);

	void SetActualType(StringHandle varname, Metadata::EpochTypeID type);

	void* GetStartOfLocals() const
	{ return StartOfLocals; }

	void* GetStartOfParams() const
	{ return StartOfParams; }

// Variable manipulation interface
public:
	template <typename T>
	T Read(StringHandle variableid)
	{
		if(OriginalScope.IsReferenceByID(variableid))
			return *reinterpret_cast<T*>(GetReferenceTargetByName(variableid));
		else
			return *reinterpret_cast<T*>(GetVariableStorageLocation(variableid));
	}

	template <typename T>
	void Write(StringHandle variableid, T value)
	{
		if(OriginalScope.IsReferenceByID(variableid))
			Write(GetReferenceTargetByName(variableid), value);
		else
			*reinterpret_cast<T*>(GetVariableStorageLocation(variableid)) = value;
	}

	template <typename T>
	void Write(void* storage, T value)
	{
		*reinterpret_cast<T*>(storage) = value;
	}

	void WriteFromStack(void* targetstorage, Metadata::EpochTypeID targettype, StackSpace& stack);

	void PushOntoStack(void* targetstorage, Metadata::EpochTypeID targettype, StackSpace& stack) const;
	void PushOntoStack(StringHandle variableid, StackSpace& stack) const;
	void PushOntoStackDeref(StringHandle variableid, StackSpace& stack) const;

// References
public:
	void* GetReferenceTarget(size_t index) const;
	void* GetReferenceTargetByName(StringHandle name) const;
	Metadata::EpochTypeID GetReferenceType(size_t index) const;

// Interaction with registers
public:
	void CopyToRegister(size_t index, Register& targetregister) const;

// State queries
public:
	bool HasReturnVariable() const
	{ return HasRet; }

	Metadata::EpochTypeID GetActualType(size_t index) const;

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

	void* GetVariableStorageLocationByIndex(size_t index) const
	{ return Data[index].StorageLocation; }

// Internal tracking
private:
	const ScopeDescription& OriginalScope;

	typedef std::pair<void*, Metadata::EpochTypeID> ReferenceStorageAndType;
	struct RuntimeData
	{
		RuntimeData()
			: StorageLocation(NULL),
			  RefInfo(std::make_pair((void*)(NULL), Metadata::EpochType_Error)),
			  ActualType(Metadata::EpochType_Error)
		{ }

		void* StorageLocation;
		ReferenceStorageAndType RefInfo;
		Metadata::EpochTypeID ActualType;
	};

	RuntimeData* Data;
	size_t DataSize;
	
	void* StartOfLocals;
	void* StartOfParams;
	size_t UsedStackSpace;

	bool HasRet;
};

