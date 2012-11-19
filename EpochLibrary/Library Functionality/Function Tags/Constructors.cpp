//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for overloaded custom constructors
//

#include "pch.h"

#include "Library Functionality/Function Tags/Constructors.h"

#include "Metadata/ScopeDescription.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"

#include "Compiler/CompileErrors.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


namespace
{

	//
	// Compile-time helper: when a variable definition is encountered, this
	// helper adds the variable itself and its type metadata to the current
	// lexical scope. This function is specifically intended for such cases
	// as overloaded structure constructors.
	//
	void ConstructorCompileHelper(IRSemantics::Statement& statement, IRSemantics::Namespace& curnamespace, IRSemantics::CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
	{
		if(statement.GetParameters()[0]->GetEpochType(curnamespace) != Metadata::EpochType_Identifier)
			throw RecoverableException("Functions tagged as constructors must accept an identifier as their first parameter");

		const IRSemantics::ExpressionAtomIdentifier* atom = dynamic_cast<const IRSemantics::ExpressionAtomIdentifier*>(statement.GetParameters()[0]->GetAtoms()[0]);

		bool shadowed = false;
		errors.SetContext(atom->GetOriginalIdentifier());
		shadowed |= curnamespace.ShadowingCheck(atom->GetIdentifier(), errors);
		shadowed |= activescope.ShadowingCheck(atom->GetIdentifier(), errors);

		if(!shadowed)
		{
			VariableOrigin origin = (inreturnexpr ? VARIABLE_ORIGIN_RETURN : VARIABLE_ORIGIN_LOCAL);
			Metadata::EpochTypeID effectivetype = curnamespace.Types.GetTypeByName(statement.GetName());
			activescope.AddVariable(curnamespace.Strings.GetPooledString(atom->GetIdentifier()), atom->GetIdentifier(), statement.GetName(), effectivetype, false, origin);
		}
	}

	//
	// Register an overloaded custom constructor
	//
	// Links the constructor function to a special compile-time helper that 
	//
	TagHelperReturn ConstructorTagHelper(StringHandle functionname, const CompileTimeParameterVector&, bool isprepass)
	{
		TagHelperReturn ret;

		if(isprepass)
		{
			ret.LinkToCompileTimeHelper = ConstructorCompileHelper;
			ret.SetConstructorFunction = functionname;
		}

		return ret;
	}

}


//
// Bind the library's tag helpers to the compiler
//
void FunctionTags::RegisterConstructorTagHelper(FunctionTagHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"constructor", &ConstructorTagHelper));
}

