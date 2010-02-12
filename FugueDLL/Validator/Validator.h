//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrappers for investigating a code tree and ensuring that it is
// valid and safe. Note that this involves more complex static
// checks than are strictly practical within the parser layer; the
// validator runs immediately after parsing and before execution.
//

#pragma once


// Forward declarations
namespace VM
{
	class Program;
	class ScopeDescription;
	class Block;
}


// Dependencies
#include "Validator/Task Safety/TaskSafety.h"


namespace Validator
{

	//
	// Track a validation error and its associated instruction
	//
	struct ValidationError
	{
		const VM::Operation* Operation;
		std::wstring ErrorText;
	};


	//
	// Helper object used with the program traversal interface
	//
	// The traversal interface takes care of invoking this helper class
	// for each element of a loaded program, such as lexical scopes,
	// individual operations, and so on.
	//
	class ValidationTraverser
	{
	// Construction
	public:
		ValidationTraverser();

	// Traversal interface
	public:
		void SetProgram(VM::Program& program);

		void TraverseGlobalInitBlock(VM::Block* block);

		template <class OperationClass>
		void TraverseNode(const OperationClass& op)
		{
			TaskSafetyCheck(op, *this);
		}

		bool EnterBlock(const VM::Block& block);
		void ExitBlock(const VM::Block& block);
		void NullBlock();

		void RegisterScope(VM::ScopeDescription& scope);

		void EnterTask();
		void ExitTask();

		void EnterThread();
		void ExitThread();

	// State query interface
	public:
		VM::ScopeDescription* GetCurrentScope()		{ return CurrentScope; }

		void SetCurrentScope(VM::ScopeDescription* scope)
		{ CurrentScope = scope; }

		bool IsValid() const						{ return Valid; }
		bool IsInTask() const						{ return (TaskDepthCounter > 0); }

	// Validation results interface
	public:
		void FlagError(const VM::Operation* op, const std::wstring& errortext);
		
		const std::list<ValidationError> GetErrorList() const
		{ return ErrorList; }

		bool HasAlreadySeenOp(const VM::Operation* op) const
		{ return (SeenOps.find(op) != SeenOps.end()); }

		void RecordTraversedOp(const VM::Operation* op)
		{ SeenOps.insert(op); }


		bool HasAlreadySeenScope(const VM::ScopeDescription& scope) const
		{ return (SeenScopes.find(&scope) != SeenScopes.end()); }

		void RecordTraversedScope(const VM::ScopeDescription& scope)
		{ SeenScopes.insert(&scope); }


		bool HasAlreadySeenBlock(const VM::Block& block) const
		{ return (SeenBlocks.find(&block) != SeenBlocks.end()); }

		void RecordTraversedBlock(const VM::Block& block)
		{ SeenBlocks.insert(&block); }

	// Internal helpers
	private:
		void TraverseScope(VM::ScopeDescription& scope);

	// Internal tracking
	private:
		bool Valid;
		
		unsigned TaskDepthCounter;

		VM::Program* CurrentProgram;
		VM::ScopeDescription* CurrentScope;

		std::list<ValidationError> ErrorList;

		std::set<const VM::Block*> SeenBlocks;
		std::set<const VM::ScopeDescription*> SeenScopes;
		std::set<const VM::Operation*> SeenOps;

	// Access to specific validation wrappers
	public:
		friend class TaskSafetyWrapper;
	};

}


