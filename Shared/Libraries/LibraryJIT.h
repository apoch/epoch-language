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

	struct JITContext
	{
		std::stack<llvm::Value*> ValuesOnStack;
		std::stack<StringHandle> ReferencesOnStack;

		std::stack<Bytecode::EntityTag> EntityTypes;

		std::map<StringHandle, llvm::Value*> VariableMap;


		llvm::BasicBlock* EntityCheck;
		llvm::BasicBlock* EntityBody;
		llvm::BasicBlock* EntityExit;

		llvm::BasicBlock* FunctionExit;


		void* Builder;
	};


	typedef void (*JITHelper)(JITContext& context);

	struct JITTable
	{
		std::map<StringHandle, JITHelper> InvokeHelpers;
		std::map<Bytecode::EntityTag, JITHelper> EntityHelpers;
	};

}

