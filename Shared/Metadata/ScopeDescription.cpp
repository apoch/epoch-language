//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing the contents of lexical scopes
//

#include "pch.h"

#include "Metadata/ScopeDescription.h"
#include "Metadata/TypeInfo.h"

#include "Utility/Strings.h"


//
// Add a variable to a lexical scope
//
void ScopeDescription::AddVariable(const std::wstring& identifier, StringHandle identifierhandle, StringHandle typenamehandle, Metadata::EpochTypeID type, bool isreference, VariableOrigin origin)
{
	if(HasVariable(identifier))
		throw InvalidIdentifierException("Duplicate/shadowed identifiers are not permitted - the identifier \"" + narrow(identifier) + "\" is already in use in this scope or some containing scope.");

	Variables.push_back(VariableEntry(identifier, identifierhandle, typenamehandle, type, isreference, origin));
}

//
// Add a variable to the beginning of a lexical scope
//
void ScopeDescription::PrependVariable(const std::wstring& identifier, StringHandle identifierhandle, StringHandle typenamehandle, Metadata::EpochTypeID type, bool isreference, VariableOrigin origin)
{
	if(HasVariable(identifier))
		throw InvalidIdentifierException("Duplicate/shadowed identifiers are not permitted - the identifier \"" + narrow(identifier) + "\" is already in use in this scope or some containing scope.");

	Variables.insert(Variables.begin(), VariableEntry(identifier, identifierhandle, typenamehandle, type, isreference, origin));
}

//
// Determine if the scope contains a variable with the given identifier
//
bool ScopeDescription::HasVariable(const std::wstring& identifier) const
{
	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->Identifier == identifier)
			return true;
	}

	if(ParentScope)
		return ParentScope->HasVariable(identifier);

	return false;
}

bool ScopeDescription::HasVariable(StringHandle identifier) const
{
	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->IdentifierHandle == identifier)
			return true;
	}

	if(ParentScope)
		return ParentScope->HasVariable(identifier);

	return false;
}

//
// Retrieve the identifier of the variable at the given index in the scope
//
const std::wstring& ScopeDescription::GetVariableName(size_t index) const
{
	return Variables[index].Identifier;
}

//
// Retrieve the string handle of the identifier of the variable at the given index in the scope
//
StringHandle ScopeDescription::GetVariableNameHandle(size_t index) const
{
	return Variables[index].IdentifierHandle;
}

//
// Retrieve the type of a variable given its identifier handle
//
Metadata::EpochTypeID ScopeDescription::GetVariableTypeByID(StringHandle variableid) const
{
	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->IdentifierHandle == variableid)
			return iter->Type;
	}

	if(ParentScope)
		return ParentScope->GetVariableTypeByID(variableid);

	throw InvalidIdentifierException("Could not retrieve the variable's type - identifier is not valid in this scope");
}

//
// Retrieve the type of the variable at the given index in the scope
//
Metadata::EpochTypeID ScopeDescription::GetVariableTypeByIndex(size_t index) const
{
	return Variables[index].Type;
}

//
// Retrieve the origin of the variable at the given index in the scope
//
VariableOrigin ScopeDescription::GetVariableOrigin(size_t index) const
{
	return Variables[index].Origin;
}

//
// Determine if the variable at the given index is a reference type
//
bool ScopeDescription::IsReference(size_t index) const
{
	return Variables[index].IsReference;
}

//
// Determine if the variable with the given identifier is a reference type
//
bool ScopeDescription::IsReferenceByID(StringHandle variableid) const
{
	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->IdentifierHandle == variableid)
			return iter->IsReference;
	}

	if(ParentScope)
		return ParentScope->IsReferenceByID(variableid);

	throw InvalidIdentifierException("Could not determine if variable is of reference type - identifier is not valid in this scope");
}

//
// Determine if the scope contains an allocated slot for a return value variable
//
bool ScopeDescription::HasReturnVariable() const
{
	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_RETURN)
			return true;
	}

	return false;
}


void ScopeDescription::Fixup(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& templateparams, const CompileTimeParameterVector& templateargs, const CompileTimeParameterVector& templateargtypes)
{
	for(VariableVector::iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		for(size_t i = 0; i < templateparams.size(); ++i)
		{
			if(iter->TypeName == templateparams[i].first)
			{
				iter->TypeName = templateargs[i].Payload.LiteralStringHandleValue;
				iter->Type = static_cast<Metadata::EpochTypeID>(templateargtypes[i].Payload.IntegerValue);
			}
		}
	}
}

bool ScopeDescription::ComputeLocalOffset(StringHandle variableid, const std::map<Metadata::EpochTypeID, size_t>& sumtypesizes, size_t& outframes, size_t& outoffset, size_t& outsize) const
{
	size_t offset = 0;

	for(ScopeDescription::VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_LOCAL || iter->Origin == VARIABLE_ORIGIN_RETURN)
		{
			size_t size = 0;
			if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_SumType || Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_Unit)
				size = sumtypesizes.find(iter->Type)->second;
			else
				size = Metadata::GetStorageSize(iter->Type);

			if(iter->IdentifierHandle == variableid)
			{
				if(!iter->IsReference)
				{
					outoffset = offset;
					outsize = size;
					return true;
				}

				return false;
			}


			offset += size;
		}
	}

	if(ParentScope)
	{
		//if(Variables.size() || Hoisted)
			++outframes;
		return ParentScope->ComputeLocalOffset(variableid, sumtypesizes, outframes, outoffset, outsize);
	}

	return false;
}

bool ScopeDescription::ComputeParamOffset(StringHandle variableid, const std::map<Metadata::EpochTypeID, size_t>& sumtypesizes, size_t& outframes, size_t& outoffset, size_t& outsize) const
{
	bool found = false;
	size_t offset = 0;

	ScopeDescription::VariableVector::const_reverse_iterator iter = Variables.rbegin();
	for(; iter != Variables.rend(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
		{
			if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_SumType)
				return false;

			size_t size = 0;
			if(iter->IsReference)
				size = sizeof(void*) + sizeof(Metadata::EpochTypeID);
			else
			{
				if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_Unit)
					size = sumtypesizes.find(iter->Type)->second;
				else
					size = Metadata::GetStorageSize(iter->Type);
			}

			if(iter->IdentifierHandle == variableid)
			{
				if(iter->IsReference)
					return false;

				outoffset = offset;
				outsize = size;
				found = true;
				break;
			}

			offset += size;
		}
	}

	for(; iter != Variables.rend(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
		{
			if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_SumType)
				return false;
		}
	}

	if(found)
		return true;

	if(ParentScope)
	{
		if(Variables.size() || Hoisted)
			++outframes;
		return ParentScope->ComputeParamOffset(variableid, sumtypesizes, outframes, outoffset, outsize);
	}

	return false;
}

size_t ScopeDescription::FindVariable(StringHandle variableid, size_t& outnumframes) const
{
	for(size_t i = 0; i < Variables.size(); ++i)
	{
		if(Variables[i].IdentifierHandle == variableid)
			return i;
	}

	if(ParentScope)
	{
		if(Variables.size() || Hoisted)
			++outnumframes;
		return ParentScope->FindVariable(variableid, outnumframes);
	}

	throw FatalException("Invalid variable ID");
}


void ScopeDescription::HoistInto(ScopeDescription* target)
{
	if(Variables.empty())
		return;

	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_RETURN)
			throw FatalException("Cannot hoist this scope, it contains a return variable!");

		//
		// Ignore identifiers that are already in this scope.
		//
		// This is safe because we do semantic shadowing checks
		// during compilation. The only time we need to actually
		// skip a variable is if the same identifier is used in
		// more than one child scope of the target scope. For
		// example: "{ integer a = 0 } { integer a = 2 }" would
		// not strictly cause shadowing (the identifier "a" is
		// unique in all scopes concerned) but once flattened
		// this can cause a collision in the target scope after
		// hoisting.
		//
		// If this "pseudo-shadowing" occurs, we can safely use
		// a single stack slot for the local variable anyways,
		// because we know it can never be used in two different
		// "meanings" via identifier aliasing.
		//
		if(target->HasVariable(iter->IdentifierHandle))
			continue;

		target->Variables.push_back(*iter);
	}

	Hoisted = true;
	Variables.clear();
}

Metadata::EpochTypeID ScopeDescription::GetReturnVariableType() const
{
	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_RETURN)
			return iter->Type;
	}

	return Metadata::EpochType_Error;
}

size_t ScopeDescription::GetParameterCount() const
{
	size_t count = 0;

	for(VariableVector::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
			++count;
	}

	return count;
}
