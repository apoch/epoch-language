//
// The Epoch Language Project
// Epoch Standard Library
//
// Conditional flow control entities
//

#include "pch.h"

#include "Library Functionality/Flow Control/Conditionals.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


using namespace FlowControl;


namespace
{
	EntityReturnCode ConditionalMetaControl(VM::ExecutionContext& context)
	{
		bool conditionalvalue = context.State.Stack.PopValue<bool>();
		if(conditionalvalue)
			return ENTITYRET_EXECUTE_CURRENT_LINK_IN_CHAIN;

		return ENTITYRET_PASS_TO_NEXT_LINK_IN_CHAIN;
	}

	EntityReturnCode ConditionalElseMetaControl(VM::ExecutionContext& context)
	{
		return ENTITYRET_EXECUTE_CURRENT_LINK_IN_CHAIN;
	}
}


//
// Register conditional flow control entities with the compiler/VM
//
void FlowControl::RegisterConditionalEntities(EntityTable& entities, EntityTable& chainedentities, StringPoolManager& stringpool)
{
	{
		EntityDescription entity;
		entity.Tag = Bytecode::EntityTags::Invalid;
		entity.MetaControl = ConditionalMetaControl;
		entity.Parameters.push_back(CompileTimeParameter(L"condition", VM::EpochType_Boolean));
		AddToMapNoDupe(entities, std::make_pair(stringpool.Pool(L"if"), entity));
	}
	{
		EntityDescription entity;
		entity.Tag = Bytecode::EntityTags::Invalid;
		entity.MetaControl = ConditionalMetaControl;
		entity.Parameters.push_back(CompileTimeParameter(L"condition", VM::EpochType_Boolean));
		AddToMapNoDupe(chainedentities, std::make_pair(stringpool.Pool(L"elseif"), entity));
	}
	{
		EntityDescription entity;
		entity.Tag = Bytecode::EntityTags::Invalid;
		entity.MetaControl = ConditionalElseMetaControl;
		AddToMapNoDupe(chainedentities, std::make_pair(stringpool.Pool(L"else"), entity));
	}
}

