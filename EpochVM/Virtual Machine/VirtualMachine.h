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
#include "Metadata/ActiveStructure.h"
#include "Metadata/StructureDefinition.h"

#include "Libraries/Library.h"

#include "Utility/Memory/Stack.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/IDTypes.h"

#include "Utility/StringPool.h"

#include "Bytecode/Instructions.h"

#include <map>
#include <set>
#include <stack>
#include <string>


// Forward declarations
class ActiveScope;
class StructureDefinition;


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
	// Construction
	public:
		VirtualMachine()
			: CurrentBufferHandle(0),
			  CurrentStructureHandle(0)
		{
		}

	// Initialization
	public:
		void InitStandardLibraries();

	// Code execution
	public:
		ExecutionResult ExecuteByteCode(const Bytecode::Instruction* buffer, size_t size);

	// Handle-managed resources
	public:
		StringHandle PoolString(const std::wstring& stringdata);
		StringHandle PoolStringDestructive(std::wstring& stringdata);
		void PoolString(StringHandle handle, const std::wstring& stringdata);
		const std::wstring& GetPooledString(StringHandle handle) const;
		StringHandle GetPooledStringHandle(const std::wstring& value);

		void* GetBuffer(BufferHandle handle);
		BufferHandle AllocateBuffer(size_t size);
		BufferHandle CloneBuffer(BufferHandle handle);

		ActiveStructure& GetStructure(StructureHandle handle);
		StructureHandle AllocateStructure(const StructureDefinition& description);

		const StructureDefinition& GetStructureDefinition(StringHandle identifier) const;

	// Functions
	public:
		void AddFunction(StringHandle name, EpochFunctionPtr funcptr);
		void AddFunction(StringHandle name, size_t instructionoffset);

		void InvokeFunction(StringHandle functionname, ExecutionContext& context);

		size_t GetFunctionInstructionOffset(StringHandle functionname) const;
		size_t GetFunctionInstructionOffsetNoThrow(StringHandle functionname) const;

	// Entity management
	public:
		void MapEntityBeginEndOffsets(size_t beginoffset, size_t endoffset);
		void MapChainBeginEndOffsets(size_t beginoffset, size_t endoffset);
		size_t GetEntityEndOffset(size_t beginoffset) const;
		size_t GetChainEndOffset(size_t beginoffset) const;

		EntityMetaControl GetEntityMetaControl(Bytecode::EntityTag tag) const;

	// Lexical scopes
	public:
		void AddLexicalScope(StringHandle name);

		const ScopeDescription& GetScopeDescription(StringHandle name) const;
		ScopeDescription& GetScopeDescription(StringHandle name);

	// Use sparingly! Only intended for access by the garbage collector
	public:
		StringPoolManager& PrivateGetRawStringPool()
		{ return PrivateStringPool; }

		const std::map<StructureHandle, ActiveStructure>& PrivateGetStructurePool()
		{ return ActiveStructures; }


	// Public tracking
	public:
		std::map<StringHandle, StructureDefinition> StructureDefinitions;

	// Handy type shortcuts
	private:
		typedef std::map<StringHandle, size_t> OffsetMap;
		typedef std::map<size_t, size_t> BeginEndOffsetMap;

	// Internal state tracking
	private:
		StringPoolManager PrivateStringPool;
		FunctionInvocationTable GlobalFunctions;
		OffsetMap GlobalFunctionOffsets;
		ScopeMap LexicalScopeDescriptions;
		EntityTable Entities;
		BeginEndOffsetMap EntityOffsets;
		BeginEndOffsetMap ChainOffsets;

		BufferHandle CurrentBufferHandle;
		std::map<BufferHandle, std::vector<Byte> > Buffers;

		StructureHandle CurrentStructureHandle;
		std::map<StructureHandle, ActiveStructure> ActiveStructures;
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
		std::stack<StringHandle> InvokedFunctionStack;

	// Internal helpers for garbage collection
	public:
		void CollectGarbage();

		void CollectGarbage_Buffers();
		void CollectGarbage_Strings();
		void CollectGarbage_Structures();

		void TickBufferGarbageCollector();
		void TickStringGarbageCollector();
		void TickStructureGarbageCollector();

	// Internal state
	private:
		const Bytecode::Instruction* CodeBuffer;
		size_t CodeBufferSize;
		size_t InstructionOffset;

		std::stack<size_t> InstructionOffsetStack;

		size_t GarbageTick_Buffers;
		size_t GarbageTick_Strings;
		size_t GarbageTick_Structures;

		std::set<StringHandle> StaticallyReferencedStrings;
	};

}

