//
// The Epoch Language Project
// Epoch Standard Library
//
// Loop flow control entities
//

#include "pch.h"

#include "Library Functionality/Flow Control/Loops.h"

#include "Runtime/Runtime.h"

#include "Utility/NoDupeMap.h"


StringHandle WhileHandle = 0;
StringHandle DoHandle = 0;


namespace
{
	bool WhileJIT(JIT::JITContext& context, bool entry)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);
		if(entry)
		{
			context.EntityBodies.push(llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "whileloop", context.InnerFunction));
			builder->CreateCondBr(context.ValuesOnStack.top(), context.EntityBodies.top(), context.EntityChains.top());
			context.ValuesOnStack.pop();
			builder->SetInsertPoint(context.EntityBodies.top());
		}
		else
		{
			context.EntityBodies.pop();
			builder->CreateBr(context.EntityChecks.top());
		}

		return true;
	}

}

static std::map<StringHandle, Bytecode::EntityTag> EntityMap;


//
// Register loop flow control entities with the compiler/VM
//
void FlowControl::RegisterLoopEntities(EntityTable& entities, EntityTable&, EntityTable& postfixentities, EntityTable& postfixclosers, Bytecode::EntityTag& tagindex)
{
	{
		EntityDescription entity;
		entity.StringName = WhileHandle;
		entity.Parameters.push_back(CompileTimeParameter(L"condition", Metadata::EpochType_Boolean));
		EntityMap[entity.StringName] = ++tagindex;
		AddToMapNoDupe(entities, std::make_pair(tagindex, entity));
	}
	{
		EntityDescription entity;
		entity.StringName = DoHandle;
		AddToMapNoDupe(postfixentities, std::make_pair(++tagindex, entity));

		EntityDescription closer;
		closer.StringName = WhileHandle;
		closer.Parameters.push_back(CompileTimeParameter(L"condition", Metadata::EpochType_Boolean));
		AddToMapNoDupe(postfixclosers, std::make_pair(++tagindex, closer));
	}
}

void FlowControl::RegisterLoopEntitiesJIT(Bytecode::EntityTag& tagindex)
{
	EntityMap[WhileHandle] = ++tagindex;
	EntityMap[DoHandle] = ++tagindex;
	++tagindex;		// Placeholder for do/while closer entity
}

void FlowControl::RegisterLoopsJITTable(JIT::JITTable& table)
{
	Bytecode::EntityTag tag = EntityMap[WhileHandle];
	AddToMapNoDupe(table.EntityHelpers, std::make_pair(tag, &WhileJIT));
}

