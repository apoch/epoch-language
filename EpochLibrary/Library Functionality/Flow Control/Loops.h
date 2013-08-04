//
// The Epoch Language Project
// Epoch Standard Library
//
// Loop flow control entities
//

#pragma once


// Dependencies
#include "Libraries/Library.h"


namespace FlowControl
{

	void RegisterLoopEntities(EntityTable& entities, EntityTable& chainedentities, EntityTable& postfixentities, EntityTable& postfixclosers, Bytecode::EntityTag& tagindex);
	
	void RegisterLoopEntitiesJIT(Bytecode::EntityTag& tagindex);
	void RegisterLoopsJITTable(JIT::JITTable& table);

}

