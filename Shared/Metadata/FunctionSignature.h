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

#include "Metadata/CompileTimeParams.h"

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
	
	void AddPatternMatchedParameter(Integer32 literalvalue);
	void AddPatternMatchedParameterIdentifier(StringHandle identifier);
	
	void SetReturnType(VM::EpochTypeID type);
	
// Inspection interface
public:
	const CompileTimeParameter& GetParameter(unsigned index) const;

	VM::EpochTypeID GetReturnType() const
	{ return ReturnType; }

	size_t GetNumParameters() const
	{ return Parameters.size(); }

// Internal tracking
private:
	std::vector<CompileTimeParameter> Parameters;

	VM::EpochTypeID ReturnType;
};


typedef std::multimap<StringHandle, FunctionSignature> FunctionSignatureSet;

