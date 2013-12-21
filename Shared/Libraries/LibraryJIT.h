#pragma once


#include <stack>
#include <map>

#include "Bytecode/EntityTags.h"

#include "Utility/Types/IDTypes.h"


class ScopeDescription;


namespace llvm
{
	class Value;
	class BasicBlock;
}

namespace JIT
{

	class NativeCodeGenerator;


	enum JITFunctionID
	{
		JITFunc_Intrinsic_VAStart,
		JITFunc_Intrinsic_VAEnd,
		JITFunc_Intrinsic_GCRoot,
		JITFunc_Intrinsic_Sqrt,

		JITFunc_Runtime_TriggerGC,
		JITFunc_Runtime_Break,
		JITFunc_Runtime_Halt,
		JITFunc_Runtime_HaltExt,
		JITFunc_Runtime_AllocStruct,
		JITFunc_Runtime_CopyStruct,
		JITFunc_Runtime_GetBuffer,
		JITFunc_Runtime_PtrToBufferHandle,
		JITFunc_Runtime_GetString,
		JITFunc_Runtime_AllocBuffer,
		JITFunc_Runtime_CopyBuffer,
		JITFunc_Runtime_PoolString,
		JITFunc_Runtime_GCBookmark,
		JITFunc_Runtime_GCUnbookmark,

		JITFunc_Marshal_ConvertStructure,
		JITFunc_Marshal_FixupStructure,
		JITFunc_Marshal_Cleanup,
		JITFunc_Marshal_GenCallback,

		JITFunc_Profile_Enter,
		JITFunc_Profile_Exit,
		JITFunc_Profile_Dump,
	};


	struct JITContext
	{
		StringHandle FunctionAlias;

		std::stack<llvm::Value*> ValuesOnStack;

		std::stack<Bytecode::EntityTag> EntityTypes;

		std::map<size_t, llvm::Value*> VariableMap;
		std::map<StringHandle, size_t> NameToIndexMap;
		std::map<size_t, llvm::Value*> SumTypeTagMap;

		llvm::Module* MyModule;

		std::stack<llvm::BasicBlock*> EntityChecks;
		std::stack<llvm::BasicBlock*> EntityBodies;
		std::stack<llvm::BasicBlock*> EntityChains;
		std::stack<llvm::BasicBlock*> EntityChainExits;
		std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*> > CheckOrphanBlocks;

		JIT::NativeCodeGenerator* Generator;

		llvm::Function* InnerFunction;
		llvm::Value* InnerRetVal;
		llvm::BasicBlock* InnerExitBlock;

		std::map<JITFunctionID, llvm::Function*>* BuiltInFunctions;

		std::map<llvm::Value*, llvm::Value*> BufferLookupCache;

		void* Context;
		void* Builder;

		llvm::Value* VarArgList;

		const ScopeDescription* CurrentScope;
	};


	typedef bool (*JITHelper)(JITContext& context, bool entry);

	struct JITTable
	{
		std::map<StringHandle, JITHelper> InvokeHelpers;
		std::map<Bytecode::EntityTag, JITHelper> EntityHelpers;
		std::map<StringHandle, const char*> LibraryExports;
	};

}

