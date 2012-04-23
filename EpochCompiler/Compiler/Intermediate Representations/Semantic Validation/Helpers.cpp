//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Semantic validation helper routines
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"


namespace IRSemantics
{

	//
	// Infer the type of a variable access
	//
	// Supports deducing the type of expressions such as "a.b.foo"
	// as well as atomic identifiers "bar"/"baz".
	//
	VM::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Program& program, const CodeBlock& activescope)
	{
		if(accesslist.empty())
			return VM::EpochType_Error;

		std::vector<StringHandle>::const_iterator iter = accesslist.begin();
		VM::EpochTypeID thetype = activescope.GetScope()->GetVariableTypeByID(*iter);

		while(++iter != accesslist.end())
		{
			StringHandle structurename = program.GetNameOfStructureType(thetype);			
			thetype = program.GetStructureMemberType(structurename, *iter);
		}

		return thetype;
	}

}

