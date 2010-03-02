//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Representation of a function's signature. Used for higher-order
// functions and polymorphism.
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Types/FunctionSignature.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/VMExceptions.h"

#include "Parser/Error Handling/ParserExceptions.h"


using namespace VM;


//
// Copy constructor
//
FunctionSignature::FunctionSignature(const FunctionSignature& rhs)
{
	CopyFrom(rhs);
}


//
// Destruct and clean up a function signature
//
FunctionSignature::~FunctionSignature()
{
	Clean();
}


//
// Copy operation
//
FunctionSignature& FunctionSignature::operator = (const FunctionSignature& rhs)
{
	if(this != &rhs) { Clean(); CopyFrom(rhs); }
	return *this;
}

//
// Copy helper
//
void FunctionSignature::CopyFrom(const FunctionSignature& rhs)
{
	Params = rhs.Params;
	Returns = rhs.Returns;
	ParamTypeHints = rhs.ParamTypeHints;
	ParamFlags = rhs.ParamFlags;

	FunctionSignatures.reserve(rhs.FunctionSignatures.size());
	for(std::vector<FunctionSignature*>::const_iterator iter = rhs.FunctionSignatures.begin(); iter != rhs.FunctionSignatures.end(); ++iter)
	{
		const FunctionSignature* sig = *iter;
		if(sig)
			FunctionSignatures.push_back(new FunctionSignature(**iter));
		else
			FunctionSignatures.push_back(NULL);
	}

	ReturnTypeHints = rhs.ReturnTypeHints;
}


//
// Add a parameter to the function signature
//
void FunctionSignature::AddParam(EpochVariableTypeID type, IDType typehint, FunctionSignature* signaturehint)
{
	ParamTypeFlags flags = PARAMTYPEFLAG_NONE;

	Params.push_back(type);
	ParamFlags.push_back(flags);
	ParamTypeHints.push_back(typehint);
	FunctionSignatures.push_back(signaturehint);
}


//
// Flag the most recently added parameter as being a reference
//
void FunctionSignature::SetLastParamToReference()
{
	if(FunctionSignatures.back())
		throw Parser::SyntaxException("Cannot pass functions by reference");

	ParamFlags.back() |= PARAMTYPEFLAG_ISREFERENCE;
}


//
// Add a return variable to the function signature
//
void FunctionSignature::AddReturn(EpochVariableTypeID type, IDType typehint)
{
	Returns.push_back(type);
	ReturnTypeHints.push_back(typehint);
}


//
// Determine if the given function matches the signature
//
bool FunctionSignature::DoesFunctionMatchSignature(const FunctionBase* function, const ScopeDescription& scope) const
{
	unsigned i = 0;

	const std::vector<std::wstring>& params = function->GetParams().GetMemberOrder();
	if(params.size() != Params.size())
		return false;
	for(std::vector<std::wstring>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
	{
		EpochVariableTypeID type = function->GetParams().GetVariableType(*iter);
		if(type != Params[i])
			return false;

		if(type == EpochVariableType_Tuple)
		{
			if(function->GetParams().GetVariableTupleTypeID(i) != ParamTypeHints[i])
				return false;
		}
		else if(type == EpochVariableType_Structure)
		{
			if(function->GetParams().GetVariableStructureTypeID(i) != ParamTypeHints[i])
				return false;
		}
		else if(type == EpochVariableType_Function)
		{
			if(!function->GetParams().GetFunctionSignature(i).DoesSignatureMatch(*FunctionSignatures[i]))
				return false;
		}
		
		++i;
	}

	const Function* epochfunc = dynamic_cast<const Function*>(function);
	if(epochfunc)
	{
		i = 0;
		const std::vector<std::wstring>& returns = epochfunc->GetReturns().GetMemberOrder();
		if(returns.size() != Returns.size())
			return false;
		for(std::vector<std::wstring>::const_iterator iter = returns.begin(); iter != returns.end(); ++iter)
		{
			EpochVariableTypeID type = epochfunc->GetReturns().GetVariableType(*iter);
			if(type != Returns[i])
				return false;

			if(type == EpochVariableType_Tuple)
			{
				if(epochfunc->GetReturns().GetVariableTupleTypeID(i) != ReturnTypeHints[i])
					return false;
			}
			else if(type == EpochVariableType_Structure)
			{
				if(epochfunc->GetReturns().GetVariableStructureTypeID(i) != ReturnTypeHints[i])
					return false;
			}

			++i;
		}
		return true;
	}

	if(Returns.empty())
		return (function->GetType(scope) == EpochVariableType_Null);
	else if(Returns.size() == 1)
		return (function->GetType(scope) == Returns[0]);

	return false;
}


//
// Test if two function signatures are equivalent
//
bool FunctionSignature::DoesSignatureMatch(const FunctionSignature& signature) const
{
	if(Params != signature.Params)
		return false;

	if(Returns != signature.Returns)
		return false;

	if(ParamTypeHints != signature.ParamTypeHints)
		return false;

	if(ParamFlags != signature.ParamFlags)
		return false;

	if(ReturnTypeHints != signature.ReturnTypeHints)
		return false;

	if(FunctionSignatures.size() != signature.FunctionSignatures.size())
		return false;

	for(unsigned i = 0; i < FunctionSignatures.size(); ++i)
	{
		bool isnull = (FunctionSignatures[i] == NULL);
		bool otherisnull = (signature.FunctionSignatures[i] == NULL);

		if(isnull != otherisnull)
			return false;

		if((!isnull) && (!FunctionSignatures[i]->DoesSignatureMatch(*signature.FunctionSignatures[i])))
			return false;
	}

	return true;
}


//
// Type hint retrieval and checking
//
IDType FunctionSignature::GetVariableTupleTypeID(unsigned index) const
{
	return ParamTypeHints[index];
}

IDType FunctionSignature::GetVariableStructureTypeID(unsigned index) const
{
	return ParamTypeHints[index];
}

const FunctionSignature& FunctionSignature::GetFunctionSignature(unsigned index) const
{
	if(!FunctionSignatures[index])
		throw ExecutionException("Requested member is not a function-typed parameter");
	return *FunctionSignatures[index];
}

EpochVariableTypeID FunctionSignature::GetVariableType(unsigned index) const
{
	return Params[index];
}

bool FunctionSignature::IsReference(unsigned index) const
{
	return ((ParamFlags[index] & PARAMTYPEFLAG_ISREFERENCE) != 0);
}

bool FunctionSignature::IsFunctionSignature(unsigned index) const
{
	return (FunctionSignatures[index] != NULL);
}


//
// Determine if this function returns a single value or a tuple
//
EpochVariableTypeID FunctionSignature::GetReturnType() const
{
	return (Returns.size() == 1 ? Returns[0] : EpochVariableType_Tuple);
}


//
// Clean up and release all data
//
void FunctionSignature::Clean()
{
	Params.clear();
	Returns.clear();
	ParamTypeHints.clear();
	ParamFlags.clear();
	
	for(std::vector<FunctionSignature*>::iterator iter = FunctionSignatures.begin(); iter != FunctionSignatures.end(); ++iter)
		delete *iter;

	FunctionSignatures.clear();

	ReturnTypeHints.clear();
}


bool FunctionSignature::IsArray(unsigned index) const
{
	return ((ParamFlags[index] & PARAMTYPEFLAG_ISARRAY) != 0);
}

EpochVariableTypeID FunctionSignature::GetArrayType(unsigned index) const
{
	return static_cast<EpochVariableTypeID>(ParamTypeHints[index]);
}

