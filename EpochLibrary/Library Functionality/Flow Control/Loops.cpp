//
// The Epoch Language Project
// Epoch Standard Library
//
// Loop flow control entities
//

#include "pch.h"

#include "Library Functionality/Flow Control/Loops.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


using namespace FlowControl;


namespace
{
	EntityReturnCode WhileMetaControl(VM::ExecutionContext& context)
	{
		bool conditionalvalue = context.State.Stack.PopValue<bool>();
		if(conditionalvalue)
			return ENTITYRET_EXECUTE_AND_REPEAT_ENTIRE_CHAIN;

		return ENTITYRET_SKIP_CHAIN;
	}
}


//
// Register loop flow control entities with the compiler/VM
//
void FlowControl::RegisterLoopEntities(EntityTable& entities, EntityTable& chainedentities, StringPoolManager& stringpool)
{
	{
		EntityDescription entity;
		entity.Tag = Bytecode::EntityTags::Invalid;
		entity.MetaControl = WhileMetaControl;
		entity.Parameters.push_back(CompileTimeParameter(L"condition", VM::EpochType_Boolean));
		AddToMapNoDupe(entities, std::make_pair(stringpool.Pool(L"while"), entity));
	}
}

