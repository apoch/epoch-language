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
	Parameters.push_back(CompileTimeParameter(name, type));
}

//
// Add a parameter that is used in pattern matching which has an integer literal type
//
void FunctionSignature::AddPatternMatchedParameter(Integer32 literalvalue)
{
	CompileTimeParameter ctparam(L"@@patternmatched", VM::EpochType_Integer);
	ctparam.Payload.IntegerValue = literalvalue;
	Parameters.push_back(ctparam);
}

//
// Add a parameter that is used in pattern matching which has a type of identifier
//
void FunctionSignature::AddPatternMatchedParameterIdentifier(StringHandle identifier)
{
	CompileTimeParameter ctparam(L"@@patternmatched", VM::EpochType_Identifier);
	ctparam.Payload.StringHandleValue = identifier;
	Parameters.push_back(ctparam);
}

//
// Set the return type of the function
//
void FunctionSignature::SetReturnType(VM::EpochTypeID type)
{
	ReturnType = type;
}

//
// Retrieve the parameter at the given index
//
const CompileTimeParameter& FunctionSignature::GetParameter(unsigned index) const
{
	return Parameters[index];
}

//
// Retrieve the parameter with the given name
//
const CompileTimeParameter& FunctionSignature::GetParameter(const std::wstring& name) const
{
	for(size_t i = 0; i < Parameters.size(); ++i)
	{
		if(Parameters[i].Name == name)
			return Parameters[i];
	}

	throw FatalException("Invalid parameter name");
}

