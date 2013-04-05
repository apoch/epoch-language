#pragma once


#include <stack>
#include <map>

#include "Bytecode/EntityTags.h"

#include "Utility/Types/IDTypes.h"


namespace llvm
{
	class Value;
	class BasicBlock;
}


namespace JIT
{

	enum JITFunctionID
	{
		JITFunc_Intrinsic_VAStart,
		JITFunc_Intrinsic_VAEnd,
		JITFunc_Intrinsic_Sqrt,

		JITFunc_VM_Break,
		JITFunc_VM_Halt,
		JITFunc_VM_AllocStruct,
		JITFunc_VM_CopyStruct,
		JITFunc_VM_GetBuffer,
	};


	struct JITContext
	{
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

		llvm::Value* VMContextPtr;

		llvm::Function* InnerFunction;

		std::map<JITFunctionID, llvm::Function*>* BuiltInFunctions;

		std::map<llvm::Value*, llvm::Value*> BufferLookupCache;

		void* Context;
		void* Builder;

		llvm::Value* VarArgList;
	};


	typedef void (*JITHelper)(JITContext& context, bool entry);

	struct JITTable
	{
		std::map<StringHandle, JITHelper> InvokeHelpers;
		std::map<Bytecode::EntityTag, JITHelper> EntityHelpers;
		std::map<StringHandle, const char*> LibraryExports;
	};

}

