//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Semantic validation helper routines
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <vector>


namespace IRSemantics
{

	// Forward declarations
	class Program;
	class CodeBlock;

	// Helper functions
	VM::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Program& program, const CodeBlock& activescope);

}

