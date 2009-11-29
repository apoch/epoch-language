//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class for an active lexical scope
//
// When a scope is entered in the executing program, that scope is
// "activated", creating one of these wrapper objects. Activation
// essentially takes the scope metadata provided in a scope description
// object, and creates a copy that represents the scope's actual
// bindings to storage, etc. We need to have active scopes rather
// than simply relying on the static description in order to support
// cases like recursive functions, where the same lexical scope may
// be "entered" multiple times but representing different instances
// of the variables/etc. each time.
//

#pragma once

#include "Virtual Machine/Core Entities/RValue.h"


// Forward declarations
class StackSpace;
class HeapStorage;



namespace VM
{

	// Forward declarations
	class ScopeDescription;
	class Variable;
	class Future;


	class ActivatedScope
	{
	// Construction
	public:
		explicit ActivatedScope(const ScopeDescription& scope);
		ActivatedScope(const ScopeDescription& scope, ActivatedScope* parent);

	// Stack interaction interface
	public:
		void Enter(StackSpace& stack);
		void Enter(HeapStorage& heapstorage);
		void Exit(StackSpace& stack);
		void ExitFromMachineStack();
		
		void BindToStack(StackSpace& stack);
		void BindToMachineStack(void* topofstack);

		RValuePtr PopVariableOffStack(const std::wstring& name, StackSpace& stack, bool ignorestorage);
		RValuePtr PopVariableOffStack(const std::wstring& name, Variable& var, StackSpace& stack, bool ignorestorage);

	// Ghost-references for parameter and return value scopes (see function documentation for more details)
	public:
		void GhostIntoScope(ActivatedScope& other) const;
		void PushNewGhostSet();
		void PopGhostSet();

	// Variable getters and setters
	public:
		RValuePtr GetVariableValue(const std::wstring& name) const;
		RValuePtr SetVariableValue(const std::wstring& name, RValuePtr value);

	// Type information retrieval
	public:
		EpochVariableTypeID GetVariableType(const std::wstring& name) const;
		EpochVariableTypeID GetVariableType(unsigned memberindex) const;	

	// Access to variables
	public:
		const Variable& GetVariableRef(const std::wstring& name) const				{ return LookupVariable(name); }
		Variable& GetVariableRef(const std::wstring& name)							{ return LookupVariable(name); }

		template <class VarClass>
		const VarClass& GetVariableRef(const std::wstring& name) const				{ return LookupVariable(name).CastTo<VarClass>(); }

		template <class VarClass>
		VarClass& GetVariableRef(const std::wstring& name)							{ return LookupVariable(name).CastTo<VarClass>(); }

	// Tuples
	public:
		bool HasTupleType(const std::wstring& name);

		const TupleType& GetTupleType(const std::wstring& name) const;
		const TupleType& GetTupleType(IDType tupletypeid) const;
		IDType GetTupleTypeID(const std::wstring& name) const;
		const std::wstring& GetTupleTypeID(IDType id) const;

		IDType GetVariableTupleTypeID(const std::wstring& varname) const;
		IDType GetVariableTupleTypeID(unsigned memberindex) const;

	// Structures
	public:
		bool HasStructureType(const std::wstring& name) const;

		const StructureType& GetStructureType(const std::wstring& name) const;
		const StructureType& GetStructureType(IDType structuretypeid) const;
		IDType GetStructureTypeID(const std::wstring& name) const;
		const std::wstring& GetStructureTypeID(IDType id) const;

		IDType GetVariableStructureTypeID(const std::wstring& varname) const;
		IDType GetVariableStructureTypeID(unsigned memberindex) const;

	// Function signatures
	public:
		bool IsFunctionSignature(const std::wstring& name) const;
		bool IsFunctionSignature(unsigned memberindex) const;

		const FunctionSignature& GetFunctionSignature(const std::wstring& name) const;
		const FunctionSignature& GetFunctionSignature(unsigned memberindex) const;

	// Futures
	public:
		Future* GetFuture(const std::wstring& name);

	// References
	public:
		bool IsReference(const std::wstring& name) const;
		bool IsReference(unsigned memberindex) const;

	// Functions
	public:
		void AddFunction(const std::wstring& name, FunctionBase* func);
		FunctionBase* GetFunction(const std::wstring& name) const;

	// Helpers for multiple return values
	public:
		RValuePtr GetEffectiveTuple() const;

	// Internal helpers
	private:
		Variable& LookupVariable(const std::wstring& name) const;
		void CheckForDuplicateIdentifier(const std::wstring& name) const;

	// Public information on the origins of this scope
	public:
		ActivatedScope* ParentScope;
		const ScopeDescription& GetOriginalDescription() const						{ return OriginalScope; }

	// Tracking of task information
	public:
		TaskHandle TaskOrigin;
		TaskHandle LastMessageOrigin;

		TaskHandle FindTaskOrigin() const;
		TaskHandle FindLastMessageOrigin() const;

	// Internal tracking
	private:
		const ScopeDescription& OriginalScope;

		typedef std::pair<std::wstring, ActivatedScope*> GhostVariableMapEntry;
		typedef std::map<std::wstring, ActivatedScope*> GhostVariableMap;
		std::deque<GhostVariableMap> Ghosts;

		typedef std::pair<std::wstring, Variable> VariableMapEntry;
		typedef std::map<std::wstring, Variable> VariableMap;
		VariableMap Variables;

		typedef std::pair<EpochVariableTypeID, Variable*> VariableRefDescriptor;
		typedef std::pair<std::wstring, VariableRefDescriptor> VariableRefMapEntry;
		typedef std::map<std::wstring, VariableRefDescriptor> VariableRefMap;
		VariableRefMap References;

		typedef std::map<std::wstring, Future*> FutureMap;
		FutureMap Futures;

		typedef std::pair<std::wstring, FunctionBase*> FunctionMapEntry;
		typedef std::map<std::wstring, FunctionBase*> FunctionMap;
		FunctionMap Functions;

		std::stack<size_t> StackUsage;
	};

}

