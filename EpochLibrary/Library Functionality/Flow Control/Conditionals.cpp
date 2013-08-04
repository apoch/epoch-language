//
// The Epoch Language Project
// Epoch Standard Library
//
// Conditional flow control entities
//

#include "pch.h"

#include "Library Functionality/Flow Control/Conditionals.h"

#include "Runtime/Runtime.h"

#include "Utility/NoDupeMap.h"


using namespace FlowControl;


StringHandle IfHandle = 0;
StringHandle ElseIfHandle = 0;
StringHandle ElseHandle = 0;

StringHandle ReturnHandle = 0;		// TODO - move all "return" stuff into a new home



namespace
{

	bool IfJIT(JIT::JITContext& context, bool entry)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);
		if(entry)
		{
			context.EntityBodies.push(llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "iftrue", context.InnerFunction));
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

		return true;
	}

	bool ElseIfJIT(JIT::JITContext& context, bool entry)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);
		if(entry)
		{
			context.EntityChains.pop();

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
			llvm::BasicBlock* chain = context.EntityChains.top();
			builder->SetInsertPoint(chain);
			context.CheckOrphanBlocks.push_back(std::make_pair(chain, context.EntityChainExits.top()));
		}

		return true;
	}

	bool ElseJIT(JIT::JITContext& context, bool entry)
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

		return true;
	}

	bool ReturnJIT(JIT::JITContext& context, bool)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);
		builder->CreateBr(context.InnerExitBlock);

		llvm::BasicBlock* garbage = llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "unreachable_garbage", context.InnerFunction);
		builder->SetInsertPoint(garbage);

		return true;
	}
}


static std::map<StringHandle, Bytecode::EntityTag> EntityMap;


//
// Register conditional flow control entities with the compiler/VM
//
void FlowControl::RegisterConditionalEntities(EntityTable& entities, EntityTable& chainedentities, Bytecode::EntityTag& tagindex)
{
	{
		EntityDescription entity;
		entity.StringName = IfHandle;
		entity.Parameters.push_back(CompileTimeParameter(L"condition", Metadata::EpochType_Boolean));
		EntityMap[entity.StringName] = ++tagindex;
		AddToMapNoDupe(entities, std::make_pair(tagindex, entity));
	}
	{
		EntityDescription entity;
		entity.StringName = ElseIfHandle;
		entity.Parameters.push_back(CompileTimeParameter(L"condition", Metadata::EpochType_Boolean));
		EntityMap[entity.StringName] = ++tagindex;
		AddToMapNoDupe(chainedentities, std::make_pair(tagindex, entity));
	}
	{
		EntityDescription entity;
		entity.StringName = ElseHandle;
		EntityMap[entity.StringName] = ++tagindex;
		AddToMapNoDupe(chainedentities, std::make_pair(tagindex, entity));
	}
}

void FlowControl::RegisterConditionalEntitiesJIT(Bytecode::EntityTag& tagindex)
{
	EntityMap[IfHandle] = ++tagindex;
	EntityMap[ElseIfHandle] = ++tagindex;
	EntityMap[ElseHandle] = ++tagindex;
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

	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(ReturnHandle, &ReturnJIT));
}

void FlowControl::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.SetReturnType(Metadata::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(ReturnHandle, signature));
	}
}

