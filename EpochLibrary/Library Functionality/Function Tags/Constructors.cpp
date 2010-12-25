//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for overloaded custom constructors
//

#include "pch.h"

#include "Library Functionality/Function Tags/Constructors.h"

#include "Metadata/ScopeDescription.h"

#include "Compilation/SemanticActionInterface.h"

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
	void ConstructorCompileHelper(const std::wstring& functionname, SemanticActionInterface& semantics, ScopeDescription& scope, const CompileTimeParameterVector& compiletimeparams)
	{
		if(compiletimeparams[0].Type != VM::EpochType_Identifier)
			throw RecoverableException("Functions tagged as constructors must accept an identifier as their first parameter");

		VariableOrigin origin = (semantics.IsInReturnDeclaration() ? VARIABLE_ORIGIN_RETURN : VARIABLE_ORIGIN_LOCAL);
		VM::EpochTypeID effectivetype = semantics.LookupTypeName(functionname);
		scope.AddVariable(compiletimeparams[0].StringPayload, compiletimeparams[0].Payload.StringHandleValue, effectivetype, false, origin);
	}

	//
	// Register an overloaded custom constructor
	//
	// Links the constructor function to a special compile-time helper that 
	//
	TagHelperReturn ConstructorTagHelper(StringHandle functionname, const CompileTimeParameterVector& compiletimeparams, bool isprepass)
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
	AddToMapNoDupe(table, std::make_pair(L"constructor", ConstructorTagHelper));
}

