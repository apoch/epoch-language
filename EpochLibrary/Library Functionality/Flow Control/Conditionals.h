//
// The Epoch Language Project
// Epoch Standard Library
//
// Conditional flow control entities
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


namespace FlowControl
{

	void RegisterConditionalEntities(EntityTable& entities, EntityTable& chainedentities, Bytecode::EntityTag& tagindex);

	void RegisterConditionalEntitiesJIT(Bytecode::EntityTag& tagindex);
	void RegisterConditionalJITTable(JIT::JITTable& table);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

}

