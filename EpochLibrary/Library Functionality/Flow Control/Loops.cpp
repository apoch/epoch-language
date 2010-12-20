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


namespace
{
	//
	// Meta-control for while loops
	//
	// Pops a boolean off the stack and conditionally executes the attached code block.
	// If the boolean evaluates to true, the code block is executed, and then the expression
	// passed to the entity is re-evaluated and the check is repeated. If at any time the
	// expression evaluates to false, the chain is exited.
	//
	EntityReturnCode WhileMetaControl(VM::ExecutionContext& context)
	{
		bool conditionalvalue = context.State.Stack.PopValue<bool>();
		if(conditionalvalue)
			return ENTITYRET_EXECUTE_AND_REPEAT_ENTIRE_CHAIN;

		return ENTITYRET_EXIT_CHAIN;
	}

	//
	// Meta-control for do-while loops
	//
	// Since a do-while loop must execute its code body at least once, this controller
	// simply tells the VM to run the current chain link. Actually exiting the loop
	// is handled by the postfix entity in DoWhileCloserMetaControl().
	//
	EntityReturnCode DoWhileMetaControl(VM::ExecutionContext& context)
	{
		return ENTITYRET_EXECUTE_CURRENT_LINK_IN_CHAIN;
	}

	//
	// Meta-control for exiting do-while loops
	//
	// This is a "postfix entity", which invokes the meta-control at the end of the
	// code body in addition to any meta-controls invoked at the beginning. Postfix
	// entities allow easy implementation of do-while loops where the loop condition
	// is not evaluated until after the body of code has run once. As with other
	// entity types, we simply pop a boolean off the stack (which was placed there
	// by the conditional expression in the postfix entity's parameters) and use
	// that value to determine whether to abort the loop or repeat from the top.
	//
	EntityReturnCode DoWhileCloserMetaControl(VM::ExecutionContext& context)
	{
		bool conditionalvalue = context.State.Stack.PopValue<bool>();
		if(conditionalvalue)
			return ENTITYRET_EXECUTE_AND_REPEAT_ENTIRE_CHAIN;

		return ENTITYRET_EXIT_CHAIN;
	}
}


//
// Register loop flow control entities with the compiler/VM
//
void FlowControl::RegisterLoopEntities(EntityTable& entities, EntityTable& chainedentities, EntityTable& postfixentities, EntityTable& postfixclosers, StringPoolManager& stringpool, Bytecode::EntityTag& tagindex)
{
	{
		EntityDescription entity;
		entity.StringName = stringpool.Pool(L"while");
		entity.MetaControl = WhileMetaControl;
		entity.Parameters.push_back(CompileTimeParameter(L"condition", VM::EpochType_Boolean));
		AddToMapNoDupe(entities, std::make_pair(++tagindex, entity));
	}
	{
		EntityDescription entity;
		entity.StringName = stringpool.Pool(L"do");
		entity.MetaControl = DoWhileMetaControl;
		AddToMapNoDupe(postfixentities, std::make_pair(++tagindex, entity));

		EntityDescription closer;
		closer.StringName = stringpool.Pool(L"while");
		closer.MetaControl = DoWhileCloserMetaControl;
		closer.Parameters.push_back(CompileTimeParameter(L"condition", VM::EpochType_Boolean));
		AddToMapNoDupe(postfixclosers, std::make_pair(++tagindex, closer));
	}
}

