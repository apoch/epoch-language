//
// The Epoch Language Project
// Epoch Standard Library
//
// Conditional flow control entities
//

#pragma once


// Dependencies
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace FlowControl
{

	void RegisterConditionalEntities(EntityTable& entities, EntityTable& chainedentities, StringPoolManager& stringpool, Bytecode::EntityTag& tagindex);

	void RegisterConditionalJITTable(JIT::JITTable& table, StringPoolManager& stringpool);

}

