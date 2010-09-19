//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing function interfaces
//

#include "pch.h"

#include "Metadata/FunctionSignature.h"


// TODO - finish documentation


FunctionSignature::FunctionSignature()
	: ReturnType(VM::EpochType_Void)
{
}


void FunctionSignature::AddParameter(const std::wstring& name, VM::EpochTypeID type)
{
	ParameterEntry paramentry;
	paramentry.ParameterName = name;
	paramentry.ParameterType = type;

	Parameters.push_back(paramentry);
}


void FunctionSignature::SetReturnType(VM::EpochTypeID type)
{
	ReturnType = type;
}


VM::EpochTypeID FunctionSignature::GetParameterType(unsigned index) const
{
	return Parameters[index].ParameterType;
}

const std::wstring& FunctionSignature::GetParameterName(unsigned index) const
{
	return Parameters[index].ParameterName;
}
