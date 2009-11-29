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

	enum FlowControlResult
	{
		FLOWCONTROL_NORMAL,					// Proceed with execution as normal
		FLOWCONTROL_BREAK,					// Break from the current loop
		FLOWCONTROL_EXITELSEIFWRAPPER,		// Shortcut out of an if/elseif/else chain
		FLOWCONTROL_RETURN					// Return from the current function
	};


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
		virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult) = 0;
		virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult) = 0;
		virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const = 0;
		virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const = 0;

	// Traversal interface
	public:
		template <class TraverserT>
		void TraverseExternal(TraverserT& traverser)
		{
			traverser.TraverseNode(dynamic_cast<SelfAwareBase*>(this)->GetToken(), GetNodeTraversalPayload());
			Operation* nested = GetNestedOperation();
			if(nested)
				nested->TraverseExternal(traverser);
		}

		virtual Traverser::Payload GetNodeTraversalPayload() const
		{
			Traverser::Payload payload;
			return payload;
		}

		virtual Operation* GetNestedOperation() const
		{
			return NULL;
		}
	};

	typedef std::auto_ptr<Operation> OperationPtr;
}

