//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built in bitwise operators
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
		// Operation for a bitwise OR
		//
		class BitwiseOr : public CompoundOperator, public Operation, public SelfAware<BitwiseOr>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

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
		// Operation for a bitwise AND
		//
		class BitwiseAnd : public CompoundOperator, public Operation, public SelfAware<BitwiseAnd>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

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
		// Operation for a bitwise XOR
		//
		class BitwiseXor : public Operation, public SelfAware<BitwiseXor>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }
		};

		//
		// Operation for a bitwise NOT
		//
		class BitwiseNot : public Operation, public SelfAware<BitwiseNot>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }
		};

	}

}
