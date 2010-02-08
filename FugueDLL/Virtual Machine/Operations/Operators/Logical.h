//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built in logical operators
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Operations/Operators/CompoundOperator.h"


namespace VM
{
	// Forward declarations
	class ScopeDescription;

	namespace Operations
	{

		//
		// Operation for a logical OR
		//
		class LogicalOr : public CompoundOperator, public Operation, public SelfAware<LogicalOr>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Boolean; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return SubOps.size(); }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);
		};

		//
		// Operation for a logical AND
		//
		class LogicalAnd : public CompoundOperator, public Operation, public SelfAware<LogicalAnd>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Boolean; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return SubOps.size(); }

		// Traversal interface
		protected:
			virtual void Traverse(Validator::ValidationTraverser& traverser);
		};

		//
		// Operation for a logical XOR
		//
		class LogicalXor : public Operation, public SelfAware<LogicalXor>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Boolean; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }
		};

		//
		// Operation for a logical NOT
		//
		class LogicalNot : public Operation, public SelfAware<LogicalNot>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Boolean; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }
		};

	}

}
