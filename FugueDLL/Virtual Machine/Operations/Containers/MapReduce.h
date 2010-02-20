//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Map and reduce builtin operations
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		class MapOperation : public Operation, public SelfAware<MapOperation>
		{
		// Construction and destruction
		public:
			explicit MapOperation(OperationPtr op)
				: TheOp(op.release())
			{ }

			virtual ~MapOperation();

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Array; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

			virtual Operation* GetNestedOperation() const
			{ return TheOp; }
			
		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		protected:
			Operation* TheOp;
		};


		class ReduceOperation : public Operation, public SelfAware<ReduceOperation>
		{
		// Construction and destruction
		public:
			explicit ReduceOperation(OperationPtr op)
				: TheOp(op.release())
			{ }

			virtual ~ReduceOperation();

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return TheOp->GetType(scope); }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

			virtual Operation* GetNestedOperation() const
			{ return TheOp; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		protected:
			Operation* TheOp;
		};

	}

}

