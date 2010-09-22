//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Declaration of the VM wrapper class
//

#pragma once


// Dependencies
#include "Metadata/ScopeDescription.h"
#include "Metadata/Register.h"

#include "Libraries/Library.h"

#include "Utility/Memory/Stack.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/IDTypes.h"

#include "Utility/StringPool.h"

#include "Bytecode/Instructions.h"

#include <map>
#include <stack>
#include <string>


// Forward declarations
class ActiveScope;


namespace VM
{

	//
	// Helper structure for storing information about a completed execution run
	//
	struct ExecutionResult
	{
		enum ExecResultType
		{
			EXEC_RESULT_OK,			// Code terminated normally
			EXEC_RESULT_HALT,		// VM encountered a HALT instruction
		};

		ExecResultType ResultType;
	};


	//
	// Wrapper for an instance of a single virtual "machine"
	//
	class VirtualMachine
	{
	// Initialization
	public:
		void InitStandardLibraries();

	// Code execution
	public:
		ExecutionResult ExecuteByteCode(const Bytecode::Instruction* buffer, size_t size);

	// Handle-managed resources
	public:
		StringHandle PoolString(const std::wstring& stringdata);
		void PoolString(StringHandle handle, const std::wstring& stringdata);
		const std::wstring& GetPooledString(StringHandle handle) const;
		StringHandle GetPooledStringHandle(const std::wstring& value);

	// Functions
	public:
		void AddFunction(StringHandle name, EpochFunctionPtr funcptr);
		void AddFunction(StringHandle name, size_t instructionoffset);

		void InvokeFunction(StringHandle functionname, ExecutionContext& context);

		size_t GetFunctionInstructionOffset(StringHandle functionname) const;

	// Lexical scopes
	public:
		void AddLexicalScope(StringHandle name);

		const ScopeDescription& GetScopeDescription(StringHandle name) const;
		ScopeDescription& GetScopeDescription(StringHandle name);

	// Internal state tracking
	private:
		StringPoolManager StringPool;
		FunctionInvocationTable GlobalFunctions;
		std::map<StringHandle, size_t> GlobalFunctionOffsets;
		std::map<StringHandle, ScopeDescription> LexicalScopeDescriptions;
	};


	//
	// Helper class for isolating a running chunk of code on a VM
	//
	struct MachineState
	{
		ExecutionResult Result;
		StackSpace Stack;
		Register ReturnValueRegister;
	};

	class ExecutionContext
	{
	// Construction
	public:
		ExecutionContext(VirtualMachine& owner, const Bytecode::Instruction* codebuffer, size_t codesize);

	// Execution interface
	public:
		void Execute(const ScopeDescription* scope);
		void Execute(size_t offset, const ScopeDescription& scope);

	// State accessors
	public:
		ExecutionResult GetExecutionResult() const
		{ return State.Result; }

	// Internal helpers for initializing the context
	private:
		void Load();

	// Internal helpers for working with the bytecode stream
	private:
		template <typename T>
		T Fetch()
		{
			const T* data = reinterpret_cast<const T*>(&CodeBuffer[InstructionOffset]);
			InstructionOffset += sizeof(T);
			return static_cast<T>(*data);
		}

		template <>
		std::wstring Fetch()
		{
			const wchar_t* data = reinterpret_cast<const wchar_t*>(&CodeBuffer[InstructionOffset]);
			std::wstring strvalue(data);
			InstructionOffset += (strvalue.length() + 1) * sizeof(wchar_t);
			return strvalue;
		}

	// Available state (for functions to operate on)
	public:
		MachineState State;
		ActiveScope* Variables;
		VirtualMachine& OwnerVM;

	// Internal state
	private:
		const Bytecode::Instruction* CodeBuffer;
		size_t CodeBufferSize;
		size_t InstructionOffset;

		std::stack<size_t> InstructionOffsetStack;
	};

}

