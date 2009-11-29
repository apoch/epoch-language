//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations and auxiliary functions for asynchronous task support
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{
	// Forward declarations
	class Block;
	class ResponseMapEntry;


	namespace Operations
	{

		//
		// Operation for forking a task
		//
		class ForkTask : public Operation, public SelfAware<ForkTask>
		{
		// Construction and destruction
		public:
			ForkTask(VM::Block* block)
				: CodeBlock(block)
			{ }

			virtual ~ForkTask();
		
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		private:
			VM::Block* CodeBlock;
		};

	}

}
