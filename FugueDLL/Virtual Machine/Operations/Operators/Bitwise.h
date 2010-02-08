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
		// Construction
		public:
			BitwiseOr(EpochVariableTypeID type);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return Type; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return SubOps.size(); }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Additional accessors
		public:
			EpochVariableTypeID GetType() const			// Overload used primarily by serialization logic
			{ return Type; }

		// Internal tracking
		protected:
			EpochVariableTypeID Type;
		};

		//
		// Operation for a bitwise AND
		//
		class BitwiseAnd : public CompoundOperator, public Operation, public SelfAware<BitwiseAnd>
		{
		// Construction
		public:
			BitwiseAnd(EpochVariableTypeID type);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

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

		// Additional accessors
		public:
			EpochVariableTypeID GetType() const			// Overload used primarily by serialization logic
			{ return Type; }

		// Internal tracking
		protected:
			EpochVariableTypeID Type;
		};

		//
		// Operation for a bitwise XOR
		//
		class BitwiseXor : public Operation, public SelfAware<BitwiseXor>
		{
		// Construction
		public:
			BitwiseXor(EpochVariableTypeID type);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return Type; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }

		// Additional accessors
		public:
			EpochVariableTypeID GetType() const			// Overload used primarily by serialization logic
			{ return Type; }
			
		// Internal tracking
		protected:
			EpochVariableTypeID Type;
		};

		//
		// Operation for a bitwise NOT
		//
		class BitwiseNot : public Operation, public SelfAware<BitwiseNot>
		{
		// Construction
		public:
			BitwiseNot(EpochVariableTypeID type);
			
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return Type; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Additional accessors
		public:
			EpochVariableTypeID GetType() const			// Overload used primarily by serialization logic
			{ return Type; }
			
		// Internal tracking
		protected:
			EpochVariableTypeID Type;
		};

	}

}
