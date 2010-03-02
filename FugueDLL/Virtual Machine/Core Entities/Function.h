//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class for encapsulating a function
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/SelfAware.h"


// Forward declarations
class StackSpace;


namespace VM
{
	// Forward declarations
	class ScopeDescription;

	//
	// Base class for all callable functions
	//
	class FunctionBase
	{
	// Destruction
	public:
		virtual ~FunctionBase()
		{ }

	// Function interface
	public:
		virtual RValuePtr Invoke(ExecutionContext& context) = 0;
		virtual const ScopeDescription& GetParams() const = 0;
		virtual ScopeDescription& GetParams() = 0;
		virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const = 0;
		virtual EpochVariableTypeID GetTypeHint(const ScopeDescription& scope) const = 0;
	};

	//
	// Class for encapsulating a user-defined Epoch function
	//
	class Function : public FunctionBase, public SelfAware<Function>
	{
	// Construction and destruction
	public:
		Function(Program& program, Block* codeblock, ScopeDescription* params, ScopeDescription* returns);
		virtual ~Function();

	// Function interface
	public:
		virtual RValuePtr Invoke(ExecutionContext& context);
		virtual RValuePtr InvokeWithExternalParams(ExecutionContext& context, void* externalstack);

		virtual ScopeDescription& GetParams()
		{ return *Params; }

		virtual const ScopeDescription& GetParams() const
		{ return *Params; }

		virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;
		virtual EpochVariableTypeID GetTypeHint(const ScopeDescription& scope) const;

	// Helpers for working with the parser
	public:
		ScopeDescription& GetReturns()
		{ return *Returns; }

		const ScopeDescription& GetReturns() const
		{ return *Returns; }
			
		void SetCodeBlock(Block* block)
		{ delete CodeBlock; CodeBlock = block; }

		const Block* GetCodeBlock() const
		{ return CodeBlock; }

	// Traversal
	public:
		template <typename TraverserT>
		void TraverseHelper(TraverserT& traverser);

		virtual void Traverse(Validator::ValidationTraverser& traverser);
		virtual void Traverse(Serialization::SerializationTraverser& traverser);

	// Access to linked program
	public:
		Program& GetRunningProgram()
		{ return *RunningProgram; }

	// Internal tracking
	protected:
		Block* CodeBlock;
		ScopeDescription* Params;
		ScopeDescription* Returns;
		Program* RunningProgram;
	};

}

