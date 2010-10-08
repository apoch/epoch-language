//
// The Epoch Language Project
// Epoch Standard Library
//
// Loop flow control entities
//

#pragma once


// Dependencies
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace FlowControl
{

	void RegisterLoopEntities(EntityTable& entities, EntityTable& chainedentities, EntityTable& postfixentities, EntityTable& postfixclosers, StringPoolManager& stringpool);

}

