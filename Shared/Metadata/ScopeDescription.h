//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing the contents of lexical scopes
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IDTypes.h"

#include <string>
#include <vector>


enum VariableOrigin
{
	VARIABLE_ORIGIN_LOCAL,
	VARIABLE_ORIGIN_PARAMETER,
	VARIABLE_ORIGIN_RETURN
};


class ScopeDescription
{
// Construction
public:
	ScopeDescription()
		: ParentScope(NULL)
	{ }

	explicit ScopeDescription(ScopeDescription* parentscope)
		: ParentScope(parentscope)
	{ }

// Configuration interface
public:
	void AddVariable(const std::wstring& identifier, StringHandle identifierhandle, VM::EpochTypeID type, VariableOrigin origin);

// Inspection interface
public:
	bool HasVariable(const std::wstring& identifier) const;

	const std::wstring& GetVariableName(size_t index) const;
	VM::EpochTypeID GetVariableTypeByID(StringHandle variableid) const;
	VM::EpochTypeID GetVariableTypeByIndex(size_t index) const;
	VariableOrigin GetVariableOrigin(size_t index) const;

	size_t GetVariableCount() const
	{ return Variables.size(); }

// Public properties
public:
	ScopeDescription* ParentScope;

// Internal helper structures
private:
	struct VariableEntry
	{
		// Constructor for convenience
		VariableEntry(const std::wstring& identifier, StringHandle identifierhandle, VM::EpochTypeID type, VariableOrigin origin)
			: Identifier(identifier),
			  IdentifierHandle(identifierhandle),
			  Type(type),
			  Origin(origin)
		{ }

		std::wstring Identifier;
		StringHandle IdentifierHandle;
		VM::EpochTypeID Type;
		VariableOrigin Origin;
	};

// Internal tracking
private:
	std::vector<VariableEntry> Variables;

// Permit activated scopes to access internal data
public:
	friend class ActiveScope;
};

