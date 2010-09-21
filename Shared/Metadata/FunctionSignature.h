//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing function interfaces
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IDTypes.h"

#include <map>
#include <vector>


class FunctionSignature
{
// Construction
public:
	FunctionSignature();

// Signature configuration interface
public:
	void AddParameter(const std::wstring& name, VM::EpochTypeID type);
	void SetReturnType(VM::EpochTypeID type);
	
// Inspection interface
public:
	VM::EpochTypeID GetParameterType(unsigned index) const;
	const std::wstring& GetParameterName(unsigned index) const;

	VM::EpochTypeID GetReturnType() const
	{ return ReturnType; }

	size_t GetNumParameters() const
	{ return Parameters.size(); }

// Internal tracking
private:
	struct ParameterEntry
	{
		std::wstring ParameterName;
		VM::EpochTypeID ParameterType;
	};

	std::vector<ParameterEntry> Parameters;

	VM::EpochTypeID ReturnType;
};


typedef std::multimap<StringHandle, FunctionSignature> FunctionSignatureSet;

