//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing the contents of lexical scopes
//

#include "pch.h"

#include "Metadata/ScopeDescription.h"


void ScopeDescription::AddVariable(const std::wstring& identifier, StringHandle identifierhandle, VM::EpochTypeID type, VariableOrigin origin)
{
	if(HasVariable(identifier))
		throw std::exception("Duplicate identifiers are not allowed");

	Variables.push_back(VariableEntry(identifier, identifierhandle, type, origin));
}

bool ScopeDescription::HasVariable(const std::wstring& identifier) const
{
	for(std::vector<VariableEntry>::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->Identifier == identifier)
			return true;
	}

	return false;
}

const std::wstring& ScopeDescription::GetVariableName(size_t index) const
{
	return Variables[index].Identifier;
}

VM::EpochTypeID ScopeDescription::GetVariableTypeByID(StringHandle variableid) const
{
	for(std::vector<VariableEntry>::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->IdentifierHandle == variableid)
			return iter->Type;
	}

	throw std::exception("Invalid variable identifier");
}

VM::EpochTypeID ScopeDescription::GetVariableTypeByIndex(size_t index) const
{
	return Variables[index].Type;
}


VariableOrigin ScopeDescription::GetVariableOrigin(size_t index) const
{
	return Variables[index].Origin;
}

