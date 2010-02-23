//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Class encapsulating a block of statements.
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/RValue.h"
#include "Virtual Machine/Core Entities/Operation.h"


// Forward declarations
class HeapStorage;


namespace VM
{
	// Forward declarations
	class ScopeDescription;
	class ActivatedScope;

	//
	// Block of sequentially executed operations
	//
	class Block
	{
	// Destruction
	public:
		Block(bool deletescopes = true)
			: DeleteScopes(deletescopes),
			  BoundScope(NULL)
		{ }

		virtual ~Block();

	// Execution interface
	public:
		virtual void ExecuteBlock(ExecutionContext& context, HeapStorage* heapstorage, bool enterscopes = true);

	// Operation-list interface
	public:
		void AddOperation(OperationPtr op);
		void ReplaceOperationFromEnd(size_t numopsfromend, OperationPtr op, const VM::ScopeDescription& scope);
		void RemoveTailOperations(size_t numops);
		void RemoveOperationFromEnd(size_t numops, const ScopeDescription& scope);
		void ShiftUpTailOperation(size_t offset);
		void ShiftUpTailOperationGroup(size_t offset, const VM::ScopeDescription& scope);
		void ReverseTailOperations(size_t numops, const ScopeDescription& scope);

		OperationPtr PopTailOperation()
		{
			OperationPtr ret(Operations.back());
			Operations.pop_back();
			return ret;
		}
		
		Operation* GetTailOperation()
		{ return *Operations.rbegin(); }

		Operation* GetOperationFromEnd(size_t numopsfromend, const ScopeDescription& scope);

		size_t GetNumOperations() const
		{ return Operations.size(); }

		void EraseOperation(Operation* op);

		const std::vector<Operation*>& GetAllOperations() const
		{ return Operations; }

		std::vector<Operation*>& GetAllOperations()
		{ return Operations; }

	// Lexical scoping interface
	public:
		void BindToScope(ScopeDescription* scope)
		{ BoundScope = scope; }

		const ScopeDescription* GetBoundScope() const
		{ return BoundScope; }

		ScopeDescription* GetBoundScope()
		{ return BoundScope; }

		void DoNotDeleteScope()
		{ DeleteScopes = false; }

	// Traversal interface
	public:
		template <class TraverserT>
		void Traverse(TraverserT& traverser)
		{
			if(traverser.EnterBlock(*this))
			{
				if(BoundScope)
					BoundScope->Traverse(traverser);
				for(std::vector<Operation*>::const_iterator iter = Operations.begin(); iter != Operations.end(); ++iter)
				{
					// We need to reset this every iteration so that the results of traversing
					// contained operations do not accidentally leave us in a different scope.
					if(BoundScope)
						traverser.SetCurrentScope(BoundScope);

					SelfAwareBase* op = dynamic_cast<SelfAwareBase*>(*iter);
					if(op)
						op->Traverse(traverser);
					else
						throw NotImplementedException("An operation class is not derived from SelfAware");
				}
				traverser.ExitBlock(*this);
			}
		}

		template <class TraverserT>
		void TraverseExternal(TraverserT& traverser) const
		{
			traverser.EnterBlock(*this);
			if(BoundScope)
				BoundScope->TraverseExternal(traverser);
			for(std::vector<Operation*>::const_iterator iter = Operations.begin(); iter != Operations.end(); ++iter)
				(*iter)->TraverseExternal(traverser);
			traverser.ExitBlock(*this);
		}

	// Additional helpers
	public:
		size_t CountTailOps(size_t numops, const ScopeDescription& scope) const;

	// Internal storage
	private:
		std::vector<Operation*> Operations;
		ScopeDescription* BoundScope;
		bool DeleteScopes;
	};
}

