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

		JITFunc_Runtime_Break,
		JITFunc_Runtime_Halt,
		JITFunc_Runtime_AllocStruct,
		JITFunc_Runtime_CopyStruct,
		JITFunc_Runtime_GetBuffer,
		JITFunc_Runtime_PtrToBufferHandle,
		JITFunc_Runtime_GetString,
		JITFunc_Runtime_AllocBuffer,
		JITFunc_Runtime_CopyBuffer,

		JITFunc_Marshal_ConvertStructure,
		JITFunc_Marshal_FixupStructure,
		JITFunc_Marshal_Cleanup,
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

		JIT::NativeCodeGenerator* Generator;

		llvm::Function* InnerFunction;
		llvm::Value* InnerRetVal;

		std::map<JITFunctionID, llvm::Function*>* BuiltInFunctions;

		std::map<llvm::Value*, llvm::Value*> BufferLookupCache;

		void* Context;
		void* Builder;

		llvm::Value* VarArgList;

		const ScopeDescription* CurrentScope;
	};


	typedef void (*JITHelper)(JITContext& context, bool entry);

	struct JITTable
	{
		std::map<StringHandle, JITHelper> InvokeHelpers;
		std::map<Bytecode::EntityTag, JITHelper> EntityHelpers;
		std::map<StringHandle, const char*> LibraryExports;
	};

}

