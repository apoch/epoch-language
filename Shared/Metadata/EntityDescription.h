//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing an inline code entity
//

#pragma once


// Dependencies
#include "Bytecode/EntityTags.h"

#include "Metadata/CompileTimeParams.h"


// Forward declarations
namespace VM
{
	class ExecutionContext;
}



enum EntityReturnCode
{
	ENTITYRET_EXIT_CHAIN,
	ENTITYRET_PASS_TO_NEXT_LINK_IN_CHAIN,
	ENTITYRET_EXECUTE_CURRENT_LINK_IN_CHAIN,
	ENTITYRET_EXECUTE_AND_REPEAT_ENTIRE_CHAIN
};


typedef EntityReturnCode (*EntityMetaControl)(VM::ExecutionContext& context);


class EntityDescription
{
// Public properties
public:
	Bytecode::EntityTag Tag;
	EntityMetaControl MetaControl;
	std::vector<CompileTimeParameter> Parameters;
};

