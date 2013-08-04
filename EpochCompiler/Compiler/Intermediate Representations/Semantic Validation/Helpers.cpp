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
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"

#include "Compiler/CompileErrors.h"

#include "Utility/StringPool.h"


namespace IRSemantics
{

	//
	// Infer the type of a variable access
	//
	// Supports deducing the type of expressions such as "a.b.foo"
	// as well as atomic identifiers such as "bar" and "baz".
	//
	Metadata::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Namespace& curnamespace, const CodeBlock& activescope, CompileErrors& errors)
	{
		if(accesslist.empty())
			return Metadata::EpochType_Error;

		std::vector<StringHandle>::const_iterator iter = accesslist.begin();
		if(!activescope.GetScope()->HasVariable(*iter))
		{
			errors.SemanticError("Variable not defined in this scope");
			return Metadata::EpochType_Error;
		}

		Metadata::EpochTypeID thetype = activescope.GetScope()->GetVariableTypeByID(*iter);

		while(++iter != accesslist.end())
		{
			StringHandle structurename = curnamespace.Types.GetNameOfType(thetype);
			thetype = curnamespace.Types.Structures.GetMemberType(structurename, *iter);
		}

		return thetype;
	}

}


//
// Compile-time helper: when a variable definition is encountered, this
// helper adds the variable itself and its type metadata to the current
// lexical scope.
//
void CompileConstructorHelper(IRSemantics::Statement& statement, IRSemantics::Namespace& curnamespace, IRSemantics::CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	const IRSemantics::ExpressionAtomIdentifierBase* atom = dynamic_cast<const IRSemantics::ExpressionAtomIdentifierBase*>(statement.GetParameters()[0]->GetAtoms()[0]);

	bool shadowed = false;
	errors.SetContext(atom->GetOriginalIdentifier());
	shadowed |= curnamespace.ShadowingCheck(atom->GetIdentifier(), errors);
	shadowed |= activescope.ShadowingCheck(atom->GetIdentifier(), errors);

	if(!shadowed)
	{
		Metadata::EpochTypeID effectivetype = curnamespace.Types.GetTypeByName(statement.GetRawName());
		VariableOrigin origin = (inreturnexpr ? VARIABLE_ORIGIN_RETURN : VARIABLE_ORIGIN_LOCAL);
		activescope.AddVariable(curnamespace.Strings.GetPooledString(atom->GetIdentifier()), atom->GetIdentifier(), statement.GetRawName(), effectivetype, origin);
	}
}
