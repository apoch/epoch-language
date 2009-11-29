//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Flow control mechanisms for the virtual machine.
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{
	// Forward declarations
	class Block;

	namespace Operations
	{
		// Forward declarations
		class ElseIfWrapper;


		//
		// Simple nested free-standing block of operations
		//
		class ExecuteBlock : public Operation, public SelfAware<ExecuteBlock>
		{
		// Construction and destruction
		public:
			ExecuteBlock(Block* body) :
				Body(body)
			{ }

			~ExecuteBlock();

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Additional helpers
		public:
			Block* Detach()
			{ Block* ret = Body; Body = NULL; return ret; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		private:
			Block* Body;
		};

		//
		// Basic true/false conditional operation
		//
		class If : public Operation, public SelfAware<If>
		{
		// Construction and destruction
		public:
			If(Block* trueblock, Block* falseblock);
			~If();

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Helper functions for loading code from the parser/deserializer
		public:
			void SetFalseBlock(Block* falseblock);
			void SetElseIfBlock(ElseIfWrapper* elseifblocks);
			ElseIfWrapper* GetElseIfBlock()
			{ return ElseIfBlocks; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		private:
			Block* TrueBlock;
			Block* FalseBlock;
			ElseIfWrapper* ElseIfBlocks;
		};


		//
		// This is a helper instruction used to properly sequence
		// operations and jumps in an if/elseif/else block.
		//
		class ElseIfWrapper : public Operation, public SelfAware<ElseIfWrapper>
		{
		// Construction and destruction
		public:
			ElseIfWrapper();
			explicit ElseIfWrapper(Block* theblock);
			~ElseIfWrapper();

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Elseif management interface
		public:
			Block* GetBlock()
			{ return WrapperBlock; }

		// Traversal interface
		public:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		private:
			Block* WrapperBlock;
		};

		//
		// One instance of this class is used per elseif clause
		//
		class ElseIf : public Operation, public SelfAware<ElseIf>
		{
		// Construction and destruction
		public:
			ElseIf(Block* block);
			~ElseIf();

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
			Block* TheBlock;
		};


		//
		// Basic do-while loop. The body is guaranteed to execute at least
		// once. Subsequent executions are controlled by the loop's
		// boolean condition expression.
		//
		class DoWhileLoop : public Operation, public SelfAware<DoWhileLoop>
		{
		// Construction and destruction
		public:
			DoWhileLoop(Block* body);
			~DoWhileLoop();

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		private:
			Block* Body;
		};


		//
		// Basic while loop. The condition is checked prior to each
		// iteration over the loop body; in contrast to a do-while
		// loop, the while loop might not ever execute at all, if the
		// condition evaluates false before the first iteration.
		//
		class WhileLoop : public Operation, public SelfAware<WhileLoop>
		{
		// Construction and destruction
		public:
			WhileLoop(Block* body);
			~WhileLoop();

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
			
		// Traversal interface
		protected:
			template <typename TraverserT>
			void TraverseHelper(TraverserT& traverser);

			virtual void Traverse(Validator::ValidationTraverser& traverser);
			virtual void Traverse(Serialization::SerializationTraverser& traverser);

		// Internal tracking
		private:
			Block* Body;
		};

		//
		// Helper operation for while loops.
		// This operation checks the conditional of the loop prior
		// to each iteration; if the condition evaluates to false,
		// the loop is exited.
		//
		class WhileLoopConditional : public Operation, public SelfAware<WhileLoopConditional>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }
		};


		//
		// Operation for breaking from a loop
		//
		class Break : public Operation, public SelfAware<Break>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};



		//
		// Operation for returning from a function
		//
		class Return : public Operation, public SelfAware<Return>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};


		//
		// Operation for branching out of an elseif clause chain
		//
		class ExitIfChain : public Operation, public SelfAware<ExitIfChain>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};
	}
}

