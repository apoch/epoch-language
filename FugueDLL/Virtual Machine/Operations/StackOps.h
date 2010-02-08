//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with the execution stack
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		//
		// Push an integer value onto the stack
		//
		class PushIntegerLiteral : public Operation, public SelfAware<PushIntegerLiteral>
		{
		// Construction
		public:
			PushIntegerLiteral(Integer32 value) :
				LiteralValue(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Additional queries
		public:
			Integer32 GetValue() const
			{ return LiteralValue; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Internal tracking
		private:
			Integer32 LiteralValue;
		};

		class PushInteger16Literal : public Operation, public SelfAware<PushInteger16Literal>
		{
		// Construction
		public:
			PushInteger16Literal(Integer16 value) :
				LiteralValue(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer16; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Additional queries
		public:
			Integer16 GetValue() const
			{ return LiteralValue; }

		// Internal tracking
		private:
			Integer16 LiteralValue;
		};

		//
		// Push a string value onto the stack
		//
		class PushStringLiteral : public Operation, public SelfAware<PushStringLiteral>
		{
		// Construction
		public:
			PushStringLiteral(const std::wstring& value) :
				LiteralValue(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_String; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Internal tracking
		private:
			std::wstring LiteralValue;
		};

		//
		// Push a fake value onto the stack
		//
		class PushRealLiteral : public Operation, public SelfAware<PushRealLiteral>
		{
		// Construction
		public:
			PushRealLiteral(Real value) :
				LiteralValue(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Real; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Internal tracking
		private:
			Real LiteralValue;
		};

		//
		// Push a boolean value onto the stack
		//
		class PushBooleanLiteral : public Operation, public SelfAware<PushBooleanLiteral>
		{
		// Construction
		public:
			PushBooleanLiteral(bool value) :
				LiteralValue(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Boolean; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Value retrieval
		public:
			bool GetValue() const
			{ return LiteralValue; }

		// Internal tracking
		private:
			bool LiteralValue;
		};

		//
		// Push the results of invoking an operation onto the stack
		//
		class PushOperation : public Operation, public SelfAware<PushOperation>
		{
		// Construction and destruction
		public:
			PushOperation(Operation* op);
			virtual ~PushOperation();

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return TheOp->GetType(scope); }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return (TheOp ? TheOp->GetNumParameters(scope) : 0); }

		// Additional queries
		public:
			virtual Operation* GetNestedOperation() const
			{ return TheOp; }

			void UnlinkOperation()
			{ TheOp = NULL; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal helpers
		public:
			static void DoPush(EpochVariableTypeID type, RValuePtr value, const ScopeDescription& scope, StackSpace& stack, bool isconslist);

		// Internal tracking
		private:
			Operation* TheOp;
			bool IsConsList;
		};


		//
		// Push a reference binding onto the stack
		//
		class BindReference : public Operation, public SelfAware<BindReference>
		{
		// Construction
		public:
			BindReference(const std::wstring& varname) :
				VarName(varname)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return VarName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Internal tracking
		private:
			const std::wstring& VarName;
		};


		//
		// Push a function binding onto the stack
		//
		class BindFunctionReference : public Operation, public SelfAware<BindFunctionReference>
		{
		// Construction
		public:
			BindFunctionReference(const std::wstring& functionname) :
				FunctionName(functionname)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return FunctionName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Internal tracking
		private:
			const std::wstring FunctionName;
		};

	}

}

