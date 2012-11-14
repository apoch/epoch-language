//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Semantic validation helper routines
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"


namespace IRSemantics
{

	//
	// Infer the type of a variable access
	//
	// Supports deducing the type of expressions such as "a.b.foo"
	// as well as atomic identifiers "bar"/"baz".
	//
	Metadata::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Namespace& curnamespace, const CodeBlock& activescope)
	{
		if(accesslist.empty())
			return Metadata::EpochType_Error;

		std::vector<StringHandle>::const_iterator iter = accesslist.begin();
		Metadata::EpochTypeID thetype = activescope.GetScope()->GetVariableTypeByID(*iter);

		while(++iter != accesslist.end())
		{
			StringHandle structurename = curnamespace.Types.GetNameOfType(thetype);
			thetype = curnamespace.Types.Structures.GetMemberType(structurename, *iter);
		}

		return thetype;
	}

}

