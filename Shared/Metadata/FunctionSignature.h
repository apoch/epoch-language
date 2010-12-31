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
	void AddParameter(const std::wstring& name, VM::EpochTypeID type, bool isreference);
	
	void AddPatternMatchedParameter(Integer32 literalvalue);
	void AddPatternMatchedParameterIdentifier(StringHandle identifier);
	
	void SetReturnType(VM::EpochTypeID type);

	void SetFunctionSignature(size_t index, const FunctionSignature& signature);

// Inspection interface
public:
	const CompileTimeParameter& GetParameter(size_t index) const;
	const CompileTimeParameter& GetParameter(const std::wstring& name) const;

	size_t FindParameter(const std::wstring& name) const;

	const FunctionSignature& GetFunctionSignature(size_t index) const;

	VM::EpochTypeID GetReturnType() const
	{ return ReturnType; }

	size_t GetNumParameters() const
	{ return Parameters.size(); }

	bool Matches(const FunctionSignature& rhs) const;

// Internal tracking
private:
	CompileTimeParameterVector Parameters;
	std::vector<FunctionSignature> FunctionSignatures;
	VM::EpochTypeID ReturnType;
};


typedef std::map<StringHandle, FunctionSignature> FunctionSignatureSet;