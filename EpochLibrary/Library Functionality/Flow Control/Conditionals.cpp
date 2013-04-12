//
// The Epoch Language Project
// Epoch Standard Library
//
// Conditional flow control entities
//

#include "pch.h"

#include "Library Functionality/Flow Control/Conditionals.h"

#include "Runtime/Runtime.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


using namespace FlowControl;


StringHandle IfHandle = 0;
StringHandle ElseIfHandle = 0;
StringHandle ElseHandle = 0;



namespace
{

	void IfJIT(JIT::JITContext& context, bool entry)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);
		if(entry)
		{
			context.EntityBodies.push(llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "", context.InnerFunction));
			builder->CreateCondBr(context.ValuesOnStack.top(), context.EntityBodies.top(), context.EntityChains.top());
			context.ValuesOnStack.pop();
			builder->SetInsertPoint(context.EntityBodies.top());
		}
		else
		{
			context.EntityBodies.pop();
			builder->CreateBr(context.EntityChainExits.top());
			builder->SetInsertPoint(context.EntityChains.top());
		}
	}

	void ElseIfJIT(JIT::JITContext& context, bool entry)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);
		if(entry)
		{
			context.EntityBodies.push(llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "elseiftrue", context.InnerFunction));
			context.EntityChains.push(llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "elseiffalse", context.InnerFunction));

			builder->CreateCondBr(context.ValuesOnStack.top(), context.EntityBodies.top(), context.EntityChains.top());
			context.ValuesOnStack.pop();
			builder->SetInsertPoint(context.EntityBodies.top());
		}
		else
		{
			context.EntityBodies.pop();
			builder->CreateBr(context.EntityChainExits.top());
		}
	}

	void ElseJIT(JIT::JITContext& context, bool entry)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);
		if(entry)
		{
			builder->SetInsertPoint(context.EntityChains.top());
		}
		else
		{
			builder->CreateBr(context.EntityChainExits.top());
		}
	}
}


static std::map<StringHandle, Bytecode::EntityTag> EntityMap;


//
// Register conditional flow control entities with the compiler/VM
//
void FlowControl::RegisterConditionalEntities(EntityTable& entities, EntityTable& chainedentities, StringPoolManager& stringpool, Bytecode::EntityTag& tagindex)
{
	{
		EntityDescription entity;
		entity.StringName = stringpool.Pool(L"if");
		entity.Parameters.push_back(CompileTimeParameter(L"condition", Metadata::EpochType_Boolean));
		EntityMap[entity.StringName] = ++tagindex;
		AddToMapNoDupe(entities, std::make_pair(tagindex, entity));
	}
	{
		EntityDescription entity;
		entity.StringName = stringpool.Pool(L"elseif");
		entity.Parameters.push_back(CompileTimeParameter(L"condition", Metadata::EpochType_Boolean));
		EntityMap[entity.StringName] = ++tagindex;
		AddToMapNoDupe(chainedentities, std::make_pair(tagindex, entity));
	}
	{
		EntityDescription entity;
		entity.StringName = stringpool.Pool(L"else");
		EntityMap[entity.StringName] = ++tagindex;
		AddToMapNoDupe(chainedentities, std::make_pair(tagindex, entity));
	}
}

void FlowControl::RegisterConditionalJITTable(JIT::JITTable& table)
{
	{
		Bytecode::EntityTag tag = EntityMap[IfHandle];
		AddToMapNoDupe(table.EntityHelpers, std::make_pair(tag, &IfJIT));
	}
	{
		Bytecode::EntityTag tag = EntityMap[ElseIfHandle];
		AddToMapNoDupe(table.EntityHelpers, std::make_pair(tag, &ElseIfJIT));
	}
	{
		Bytecode::EntityTag tag = EntityMap[ElseHandle];
		AddToMapNoDupe(table.EntityHelpers, std::make_pair(tag, &ElseJIT));
	}
}

