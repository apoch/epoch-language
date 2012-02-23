// TODO - documentation
#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

namespace IRSemantics
{

	VM::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Program& program, const CodeBlock& activescope)
	{
		if(accesslist.empty())
			return VM::EpochType_Error;

		std::vector<StringHandle>::const_iterator iter = accesslist.begin();
		VM::EpochTypeID thetype = activescope.GetScope()->GetVariableTypeByID(*iter);

		while(++iter != accesslist.end())
		{
			StringHandle structurename = program.GetNameOfStructureType(thetype);
			StringHandle memberaccessname = program.FindStructureMemberAccessOverload(structurename, *iter);
			
			thetype = program.GetStructureMemberType(structurename, memberaccessname);
		}

		return thetype;
	}

}