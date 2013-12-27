//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Declaration of the execution context wrapper class
//

#pragma once

// Dependencies
#include "Exports/ExportDef.h"

#include "Metadata/ScopeDescription.h"
#include "Metadata/ActiveStructure.h"
#include "Metadata/StructureDefinition.h"
#include "Metadata/Variant.h"

#include "Libraries/Library.h"
#include "Libraries/LibraryJIT.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/IDTypes.h"

#include "Utility/StringPool.h"

#include "Utility/Threading/Synchronization.h"

#include "Bytecode/Instructions.h"

#include "JIT/JIT.h"

#include <map>
#include <set>
#include <stack>
#include <string>

#include <boost/unordered_map.hpp>


// Forward declarations
class StructureDefinition;

struct fasthash : std::unary_function<StringHandle, size_t>
{
	size_t operator() (StringHandle a) const
	{
		a = (a ^ 61) ^ (a >> 16);
		a = a + (a << 3);
		a = a ^ (a >> 4);
		a = a * 0x27d4eb2d;
		a = a ^ (a >> 15);
		return a;
	}
};


namespace Runtime
{

	// Handy type shortcuts
	typedef boost::unordered_map<StringHandle, ScopeDescription, fasthash> ScopeMap;

	class ExecutionContext
	{
	// Constants
	public:
		enum GarbageCollectionFlags
		{
			GC_Collect_Strings		= 1 << 0,
			GC_Collect_Buffers		= 1 << 1,
			GC_Collect_Structures	= 1 << 2,
		};

	// Construction and destruction
	public:
		ExecutionContext(Bytecode::Instruction* codebuffer, size_t codesize, unsigned* testharness, bool usestdlib = true);
		~ExecutionContext();

	// Non-copyable
	private:
		ExecutionContext(const ExecutionContext& rhs);
		ExecutionContext& operator = (const ExecutionContext& rhs);

	// Execution interface
	public:
		void Execute();

	// Helpers for initializing the context
	public:
		void Load();

	// JIT support
	public:
		void* JITCallback(void* targetfunc);
		void* JITCallbackNoStub(void* targetfunc);

	// Internal helpers for working with the bytecode stream
	private:
		template <typename T>
		__forceinline T Fetch(size_t& instructionoffset)
		{
			const T* data = reinterpret_cast<const T*>(&CodeBuffer[instructionoffset]);
			instructionoffset += sizeof(T);
			return static_cast<T>(*data);
		}

		template <>
		std::wstring Fetch(size_t& instructionoffset)
		{
			const wchar_t* data = reinterpret_cast<const wchar_t*>(&CodeBuffer[instructionoffset]);
			std::wstring strvalue(data);
			instructionoffset += (strvalue.length() + 1) * sizeof(wchar_t);
			return strvalue;
		}

	// Available state (for functions to operate on)
	public:
		boost::unordered_set<StringHandle> StaticallyReferencedStrings;

	// Internal helpers for garbage collection
	public:
		EPOCHRUNTIME void TickBufferGarbageCollector();
		EPOCHRUNTIME void TickStructureGarbageCollector();

	// Internal state
	private:
	    Bytecode::Instruction* CodeBuffer;
		size_t CodeBufferSize;

		size_t GarbageTick_Buffers;
		size_t GarbageTick_Structures;

		std::map<void*, void*> TargetCallbackToJITFuncMap;


	// TODO - cleanup/reorganize

	// Profiling
	public:
		void ProfileEnter(StringHandle funcname);
		void ProfileExit(StringHandle funcname);
		void ProfileDump();

		std::map<StringHandle, std::vector<unsigned> > ProfilingData;
		std::map<StringHandle, std::pair<unsigned, unsigned> > ProfilingTimes;

	// Initialization
	private:
		void InitStandardLibraries(unsigned* testharness, bool registerall);

	// Code execution
	public:
		void ExecuteByteCode();

	// Handle-managed resources
	public:
		EPOCHRUNTIME StringHandle PoolString(const std::wstring& stringdata);
		void PoolString(StringHandle handle, const std::wstring& stringdata);
		EPOCHRUNTIME StringHandle PoolString(const wchar_t* stringdata);
		EPOCHRUNTIME const std::wstring& GetPooledString(StringHandle handle) const;
		StringHandle GetPooledStringHandle(const std::wstring& value);

		EPOCHRUNTIME void* GetBuffer(BufferHandle handle);
		EPOCHRUNTIME BufferHandle FindBuffer(const void* ptr);
		EPOCHRUNTIME size_t GetBufferSize(BufferHandle handle) const;
		EPOCHRUNTIME BufferHandle AllocateBuffer(size_t size);
		BufferHandle CloneBuffer(BufferHandle handle);

		StructureHandle AllocateStructure(const StructureDefinition& description);
		StructureHandle DeepCopy(StructureHandle handle);
		EPOCHRUNTIME ActiveStructure& FindStructureMetadata(StructureHandle handle);

		EPOCHRUNTIME const StructureDefinition& GetStructureDefinition(Metadata::EpochTypeID vartype) const;
		bool HasStructureDefinition(Metadata::EpochTypeID vartype) const;

	// Functions
	public:
		void AddFunction(StringHandle name, EpochFunctionPtr funcptr);
		void AddFunction(StringHandle name, size_t instructionoffset);

		size_t GetFunctionInstructionOffset(StringHandle functionname) const;
		size_t GetFunctionInstructionOffsetNoThrow(StringHandle functionname) const;

		const FunctionSignature& GetFunctionSignatureByType(Metadata::EpochTypeID type) const;

	// Entity management
	public:
		void MapEntityBeginEndOffsets(size_t beginoffset, size_t endoffset);
		void MapChainBeginEndOffsets(size_t beginoffset, size_t endoffset);
		size_t GetEntityEndOffset(size_t beginoffset) const;
		size_t GetChainEndOffset(size_t beginoffset) const;

	// Lexical scopes
	public:
		void AddLexicalScope(StringHandle name);

		const ScopeDescription& GetScopeDescription(StringHandle name) const;
		ScopeDescription& GetScopeDescription(StringHandle name);

	// Garbage collection
	public:
		void GarbageCollectBuffers(const std::set<BufferHandle>& livehandles);
		void GarbageCollectStructures(const std::set<StructureHandle>& livehandles);

		unsigned GetGarbageCollectionBitmask();

	// Use sparingly! Only intended for access by the garbage collector
	public:
		StringPoolManager& PrivateGetRawStringPool()
		{ return PrivateStringPool; }

	// Public tracking
	public:
		EntityTable Entities;

		std::map<Metadata::EpochTypeID, StructureDefinition> StructureDefinitions;
		JIT::JITTable JITHelpers;
		void* EntryPointFunc;
		void* GlobalInitFunc;
		void* TriggerGCFunc;

		boost::unordered_map<Metadata::EpochTypeID, VariantDefinition, fasthash> VariantDefinitions;

		std::map<StringHandle, size_t> PatternMatcherParamCount;
		std::map<StringHandle, Metadata::EpochTypeID> PatternMatcherRetType;
		std::map<StringHandle, StringHandle> PatternMatcherDispatchHint;

		std::map<StringHandle, size_t> TypeMatcherParamCount;
		std::map<StringHandle, Metadata::EpochTypeID> TypeMatcherRetType;

		FunctionSignatureSet LibraryFunctionSignatures;

		std::set<StringHandle> AutoGeneratedConstructors;

		std::map<Metadata::EpochTypeID, FunctionSignature> FunctionTypeToSignatureMap;

		std::map<void*, std::pair<void*, StringHandle> > GeneratedJITFunctionCode;
		std::map<void*, void*> GeneratedFunctionLLVMToMachineCodeMap;
		void* JITExecutionEngine;

		std::set<StringHandle> SuppressGC;

	// Handy type shortcuts
	private:
		typedef boost::unordered_map<StringHandle, size_t, fasthash> OffsetMap;
		typedef boost::unordered_map<size_t, size_t, fasthash> BeginEndOffsetMap;

	// Internal state tracking
	private:
		StringPoolManager PrivateStringPool;
		FunctionInvocationTable GlobalFunctions;
		OffsetMap GlobalFunctionOffsets;
		ScopeMap LexicalScopeDescriptions;
		BeginEndOffsetMap EntityOffsets;
		BeginEndOffsetMap ChainOffsets;

		HandleAllocator<BufferHandle> BufferHandleAlloc;
		boost::unordered_map<BufferHandle, std::vector<Byte>, fasthash> Buffers;

		UByte* StructureAllocBuffer;
		UByte* StructureAllocPoint;

		UByte* MetadataAllocBuffer;
		UByte* MetadataAllocPoint;
	};

}

