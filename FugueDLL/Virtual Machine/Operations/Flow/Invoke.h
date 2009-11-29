//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation for invoking a function at runtime
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	// Forward declarations
	class FunctionBase;


	namespace Operations
	{

		//
		// Operation for invoking a given function
		//
		class Invoke : public Operation, public SelfAware<Invoke>
		{
		// Construction and destruction
		public:
			Invoke(FunctionBase* function, bool cleanupfunction);
			virtual ~Invoke();
		
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const;

		// Access interface
		public:
			FunctionBase* GetFunction() const
			{ return Function; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Internal tracking
		private:
			FunctionBase* Function;
			bool CleanUpFunction;
		};


		//
		// Operation for invoking a function indirectly given a bound variable
		// Used for higher-order functions and first-class functions primarily
		//
		class InvokeIndirect : public Operation, public SelfAware<InvokeIndirect>
		{
		// Construction
		public:
			InvokeIndirect(const std::wstring& functionname)
				: FunctionName(functionname)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const;

		// Access interface
		public:
			const std::wstring& GetFunctionName() const
			{ return FunctionName; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

			virtual Traverser::Payload GetNodeTraversalPayload() const
			{
				Traverser::Payload payload;
				payload.SetValue(FunctionName.c_str());
				payload.IsIdentifier = true;
				return payload;
			}

		// Internal tracking
		private:
			const std::wstring& FunctionName;
		};

	}

}

