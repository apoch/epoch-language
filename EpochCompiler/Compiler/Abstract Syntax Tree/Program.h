//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Node classes for constructing abstract syntax trees for Epoch programs
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"

#include "Compiler/Abstract Syntax Tree/RefCounting.h"
#include "Compiler/Abstract Syntax Tree/Forwards.h"

#include "Compiler/Abstract Syntax Tree/Undefined.h"


namespace AST
{
	typedef boost::variant
		<
			Undefined,
			DeferredStructure,
			DeferredCodeBlock,
			Deferred<Function>
		> MetaEntity;
	
	typedef std::vector<MetaEntity, Memory::OneWayAlloc<MetaEntity> > MetaEntityVector;

	struct Program
	{
		MetaEntityVector MetaEntities;

		Program()
		{ }

		Program(const MetaEntityVector& metaentities)
			: MetaEntities(metaentities)
		{ }
	};
}
