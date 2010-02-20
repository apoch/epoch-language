//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Metadata for tracking a lexical scope.
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Core Entities/RValue.h"
#include "Virtual Machine/Core Entities/Types/FunctionSignature.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"

#include "Virtual Machine/Types Management/Typecasts.h"


// Forward declarations
class StackSpace;
class HeapStorage;
namespace Validator { class ValidationTraverser; }
namespace Serialization { class SerializationTraverser; }


namespace VM
{

	// Forward declarations
	class FunctionBase;
	class Operation;
	class Block;
	class ResponseMap;
	class Future;

	typedef std::auto_ptr<Operation> OperationPtr;


	//
	// Basic lexical scope container
	//
	class ScopeDescription
	{
	// Friends of this class
	public:
		friend class FileLoader;
		friend class ActivatedScope;
		friend class Serialization::SerializationTraverser;
		friend class Validator::ValidationTraverser;

	// Construction and destruction
	public:
		ScopeDescription();
		ScopeDescription(const ScopeDescription& other);

		~ScopeDescription();

	// Variable addition interface
	public:
		void AddVariable(const std::wstring& name, EpochVariableTypeID type);
		void AddTupleVariable(const std::wstring& tupletypename, const std::wstring& name);
		void AddStructureVariable(const std::wstring& structuretypename, const std::wstring& name);
		void AddStructureVariable(IDType structuretypeid, const std::wstring& name);

	// References interface
	public:
		void AddReference(EpochVariableTypeID type, const std::wstring& name);
		bool IsReference(const std::wstring& name) const;
		bool IsReference(unsigned memberindex) const
		{ return IsReference(MemberOrder[memberindex]); }

	// Function signatures interface (for first-class functions)
	public:
		void AddFunctionSignature(const std::wstring& name, const FunctionSignature& signature, bool insertmember);
		bool IsFunctionSignature(const std::wstring& name) const;
		bool IsFunctionSignature(unsigned memberindex) const
		{ return IsFunctionSignature(MemberOrder[memberindex]); }

		const FunctionSignature& GetFunctionSignature(const std::wstring& name) const;
		const FunctionSignature& GetFunctionSignature(unsigned memberindex) const
		{ return GetFunctionSignature(MemberOrder[memberindex]); }

	// Futures interface
	public:
		void AddFuture(const std::wstring& name, VM::OperationPtr boundop);

	// Arrays interface
	public:
		void SetArrayType(const std::wstring& arrayname, EpochVariableTypeID type);
		EpochVariableTypeID GetArrayType(const std::wstring& arrayname) const;

		void SetArraySize(const std::wstring& arrayname, size_t size);
		size_t GetArraySize(const std::wstring& arrayname) const;

	// Generic variable information retrieval
	public:
		bool HasVariable(const std::wstring& name) const
		{
			for(std::vector<std::wstring>::const_iterator iter = MemberOrder.begin(); iter != MemberOrder.end(); ++iter)
			{
				if(*iter == name)
					return true;
			}

			if(Futures.find(name) != Futures.end())
				return true;

			if(ParentScope)
				return ParentScope->HasVariable(name);

			return false;
		}

		EpochVariableTypeID GetVariableType(const std::wstring& name) const;
		EpochVariableTypeID GetVariableType(unsigned memberindex) const
		{ return GetVariableType(MemberOrder[memberindex]); }

		const std::vector<std::wstring>& GetMemberOrder() const
		{ return MemberOrder; }

		size_t GetNumMembers() const
		{ return MemberOrder.size(); }

		void SetConstant(const std::wstring& name);
		bool IsConstant(const std::wstring& name) const;

		const ScopeDescription* GetScopeOwningVariable(const std::wstring& name) const;

	// Ghost-references for parameter and return value scopes (see function documentation for more details)
	public:
		void GhostIntoScope(ScopeDescription& other) const;
		void PushNewGhostSet();
		void PopGhostSet();
		std::set<const ScopeDescription*> GetAllGhostScopes() const;

	// Interface for working with functions
	public:
		void AddFunction(const std::wstring& name, std::auto_ptr<FunctionBase> func);
		FunctionBase* GetFunction(const std::wstring& name) const;

		EpochVariableTypeID GetFunctionType(const std::wstring& name) const;
		
		bool HasFunction(const std::wstring& name) const
		{
			if(Functions.find(name) != Functions.end())
				return true;

			if(ParentScope)
				return ParentScope->HasFunction(name);

			return false;
		}

		const std::wstring& GetFunctionName(const FunctionBase* func) const;

	// Helpers for functions with multiple return values
	public:
		EpochVariableTypeID GetEffectiveType() const;
		void RegisterSelfAsTupleType(const std::wstring& nameoftype);


	// Interface for working with tuples
	public:
		void AddTupleType(const std::wstring& name, const TupleType& typeinfo);

		bool HasTupleType(const std::wstring& name) const
		{
			if(TupleTypes.find(name) != TupleTypes.end())
				return true;

			if(ParentScope)
				return ParentScope->HasTupleType(name);

			return false;
		}

		const TupleType& GetTupleType(const std::wstring& name) const;
		const TupleType& GetTupleType(IDType tupletypeid) const;
		IDType GetTupleTypeID(const std::wstring& name) const;
		const std::wstring& GetTupleTypeID(IDType id) const;

		IDType GetVariableTupleTypeID(const std::wstring& varname) const;
		IDType GetVariableTupleTypeID(unsigned memberindex) const
		{ return GetVariableTupleTypeID(MemberOrder[memberindex]); }

		void SetVariableTupleTypeID(const std::wstring& name, IDType id);

	// Interface for working with structures
	public:
		void AddStructureType(const std::wstring& name, const StructureType& typeinfo);

		bool HasStructureType(const std::wstring& name) const
		{
			if(StructureTypes.find(name) != StructureTypes.end())
				return true;

			if(ParentScope)
				return ParentScope->HasStructureType(name);

			return false;
		}

		const StructureType& GetStructureType(const std::wstring& name) const;
		const StructureType& GetStructureType(IDType structuretypeid) const;
		IDType GetStructureTypeID(const std::wstring& name) const;
		const std::wstring& GetStructureTypeID(IDType id) const;

		IDType GetVariableStructureTypeID(const std::wstring& varname) const;
		IDType GetVariableStructureTypeID(unsigned memberindex) const
		{ return GetVariableStructureTypeID(MemberOrder[memberindex]); }

		void SetVariableStructureTypeID(const std::wstring& name, IDType id);

	// Message responses
	public:
		void AddResponseMap(const std::wstring& name, VM::ResponseMap* responses);
		const VM::ResponseMap& GetResponseMap(const std::wstring& name) const;

	// Traversal interface
	public:
		template <class TraverserT>
		void Traverse(TraverserT& traverser)
		{
			traverser.RegisterScope(*this);
		}

		template <class TraverserT>
		void TraverseExternal(TraverserT& traverser)
		{
			traverser.RegisterScope(this);
		}

	// Internal helpers
	private:
		Variable& LookupVariable(const std::wstring& name) const;
		void CheckForDuplicateIdentifier(const std::wstring& name) const;

	// Public properties
	public:
		ScopeDescription* ParentScope;

	// Internal tracking
	private:
		typedef std::pair<std::wstring, Variable> VariableMapEntry;
		typedef std::map<std::wstring, Variable> VariableMap;
		VariableMap Variables;

		typedef std::pair<EpochVariableTypeID, Variable*> VariableRefDescriptor;
		typedef std::pair<std::wstring, VariableRefDescriptor> VariableRefMapEntry;
		typedef std::map<std::wstring, VariableRefDescriptor> VariableRefMap;
		VariableRefMap References;

		typedef std::pair<std::wstring, ScopeDescription*> GhostVariableMapEntry;
		typedef std::map<std::wstring, ScopeDescription*> GhostVariableMap;
		std::deque<GhostVariableMap> Ghosts;

		typedef std::map<std::wstring, Future*> FutureMap;
		FutureMap Futures;

		typedef std::pair<std::wstring, FunctionBase*> FunctionMapEntry;
		typedef std::map<std::wstring, FunctionBase*> FunctionMap;
		FunctionMap Functions;

		typedef std::map<std::wstring, FunctionSignature> FunctionSignatureMap;
		typedef std::pair<std::wstring, FunctionSignature> FunctionSignatureMapEntry;
		FunctionSignatureMap FunctionSignatures;

		typedef std::map<std::wstring, IDType> TupleTypeIDMap;
		typedef std::pair<std::wstring, IDType> TupleTypeIDMapEntry;
		TupleTypeIDMap TupleTypes;
		TupleTypeIDMap TupleTypeHints;
		TupleTrackerClass TupleTracker;

		typedef std::map<std::wstring, IDType> StructureTypeIDMap;
		typedef std::pair<std::wstring, IDType> StructureTypeIDMapEntry;
		StructureTypeIDMap StructureTypes;
		StructureTypeIDMap StructureTypeHints;
		StructureTrackerClass StructureTracker;
		
		std::vector<std::wstring> MemberOrder;
		std::vector<std::wstring> Constants;
		
		typedef std::map<std::wstring, ResponseMap*> ResponseMapList;
		typedef std::pair<std::wstring, ResponseMap*> ResponseMapListEntry;
		ResponseMapList ResponseMaps;

		std::map<std::wstring, EpochVariableTypeID> ArrayTypes;
		std::map<std::wstring, size_t> ArraySizes;
	};

}
