//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Virtual machine operations for working with built-in containers
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		//
		// This operation constructs an anonymous array container on the stack
		// It is primarily useful for passing an indeterminate number of values
		// into a variadic function, such as or() or add().
		//
		class ConsArray : public Operation, public SelfAware<ConsArray>
		{
		// Construction
		public:
			ConsArray(size_t numentries, EpochVariableTypeID elementtype);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Array; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return NumEntries; }

		// Queries
		public:
			EpochVariableTypeID GetElementType() const
			{ return ElementType; }

			size_t GetNumEntries() const
			{ return NumEntries; }

		// Internal tracking
		private:
			size_t NumEntries;
			EpochVariableTypeID ElementType;
		};


		class ConsArrayIndirect : public Operation, public SelfAware<ConsArrayIndirect>
		{
		// Construction and destruction
		public:
			ConsArrayIndirect(EpochVariableTypeID elementtype, Operation* op);
			virtual ~ConsArrayIndirect();

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

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
			{
				Traverser::Payload payload;
				payload.SetValue(TheOp);
				payload.ParameterCount = 1;
				payload.InvokesFunction = true;
				return payload;
			}

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Queries
		public:
			EpochVariableTypeID GetElementType() const
			{ return ElementType; }

		// Internal tracking
		private:
			EpochVariableTypeID ElementType;
			Operation* TheOp;
		};


		//
		// Operation for retrieving a value from an array
		//
		class ReadArray : public Operation, public SelfAware<ReadArray>
		{
		// Construction
		public:
			ReadArray(const std::wstring& arrayname);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return ArrayName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const;
			
		// Internal tracking
		private:
			const std::wstring& ArrayName;
		};


		//
		// Operation for writing a value to an array
		//
		class WriteArray : public Operation, public SelfAware<WriteArray>
		{
		// Construction
		public:
			WriteArray(const std::wstring& arrayname);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return VM::EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return ArrayName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const;

		// Internal tracking
		private:
			const std::wstring& ArrayName;
		};


		//
		// Operation for retrieving the length of an array
		//
		class ArrayLength : public Operation, public SelfAware<ArrayLength>
		{
		// Construction
		public:
			ArrayLength(const std::wstring& arrayname)
				: ArrayName(arrayname)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return ArrayName; }

		// Traversal interface
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const;

		// Internal tracking
		private:
			const std::wstring& ArrayName;
		};

	}

}

