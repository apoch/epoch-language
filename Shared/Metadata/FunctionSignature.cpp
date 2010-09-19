//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing function interfaces
//

#include "pch.h"

#include "Metadata/FunctionSignature.h"


//
// Construct and initialize a function signature wrapper
//
FunctionSignature::FunctionSignature()
	: ReturnType(VM::EpochType_Void)
{
}

//
// Add a parameter with the given name and data type to the function signature
//
void FunctionSignature::AddParameter(const std::wstring& name, VM::EpochTypeID type)
{
	ParameterEntry paramentry;
	paramentry.ParameterName = name;
	paramentry.ParameterType = type;

	Parameters.push_back(paramentry);
}

//
// Set the return type of the function
//
void FunctionSignature::SetReturnType(VM::EpochTypeID type)
{
	ReturnType = type;
}

//
// Retrieve the type of the parameter at the given index
//
VM::EpochTypeID FunctionSignature::GetParameterType(unsigned index) const
{
	return Parameters[index].ParameterType;
}

//
// Retrieve the name of the parameter at the given index
//
const std::wstring& FunctionSignature::GetParameterName(unsigned index) const
{
	return Parameters[index].ParameterName;
}
