//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class for encapsulating an atomic operation
//
// Note that operations have two similar methods: ExecuteAndStoreRValue and ExecuteFast.
// In general, we use ExecuteAndStoreRValue when the operation returns some r-value that
// may be wanted for future use. When no return value is required (i.e. if no subsequent
// operation uses the operation's result), we rely on ExecuteFast, which does not create
// an RValuePtr for the return value. Avoiding excessive uses of r-values helps maintain
// better performance, especially in inner loops etc.
//

#pragma once


// Dependencies
#include "Virtual Machine/ExecutionContext.h"
#include "Virtual Machine/Core Entities/RValue.h"
#include "Virtual Machine/SelfAware.h"
#include "Traverser/TraversalInterface.h"


// Forward declarations
class StackSpace;
class DebugTable;


namespace VM
{
	// Forward declarations
	class ScopeDescription;
	class ActivatedScope;
	class Block;


	//
	// Base interface for all language operations
	//
	class Operation
	{
	// Destruction
	public:
		virtual ~Operation() { }

	// Execution and type-retrieval interface
	public:
		virtual void ExecuteFast(ExecutionContext& context) = 0;
		virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context) = 0;
		virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const = 0;
		virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const = 0;

	// Traversal interface
	public:
		template <class TraverserT>
		void TraverseExternal(TraverserT& traverser) const
		{
			traverser.TraverseNode(dynamic_cast<const SelfAwareBase*>(this)->GetToken(), GetNodeTraversalPayload(traverser.GetCurrentScope()));
			Operation* nested = GetNestedOperation();
			if(nested)
				nested->TraverseExternal(traverser);
			Block* attachedblock = GetAttachedCodeBlock();
			if(attachedblock)
				attachedblock->TraverseExternal(traverser);
		}

		virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
		{
			Traverser::Payload payload;
			payload.ParameterCount = GetNumParameters(*scope);
			return payload;
		}

		virtual Operation* GetNestedOperation() const
		{
			return NULL;
		}

		virtual Block* GetAttachedCodeBlock() const
		{
			return NULL;
		}
	};

	typedef std::auto_ptr<Operation> OperationPtr;
}

