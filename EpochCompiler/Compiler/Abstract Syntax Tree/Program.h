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

#include "Compiler/Abstract Syntax Tree/CodeBlock.h"


namespace AST
{

	//
	// A meta-entity is any top-level construct in an Epoch
	// program. Valid meta-entities include functions, structure
	// definitions, and global code blocks.
	//
	typedef boost::variant
		<
			Undefined,
			DeferredStructure,
			DeferredCodeBlockEntry,
			DeferredFunction
		> MetaEntity;
	
	//
	// A container of meta-entities
	//
	typedef std::vector<MetaEntity, Memory::OneWayAlloc<MetaEntity> > MetaEntityVector;

	//
	// A program consists of a sequence of meta-entities
	//
	// We use a separate wrapper structure to aid in forward
	// declarations of code that needs to pass AST nodes around
	// without actually invoking their members in any way.
	//
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
