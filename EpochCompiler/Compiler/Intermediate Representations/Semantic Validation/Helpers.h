#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <vector>


namespace IRSemantics
{

	class Program;
	class CodeBlock;

	VM::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Program& program, const CodeBlock& activescope);

}