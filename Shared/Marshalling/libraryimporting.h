//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Shared header file used by both the VM and external library DLLs.
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"


//
// This helper is used to inform the VM of what library routines are available
//
struct ParamData
{
	ParamData(const wchar_t* name, VM::EpochVariableTypeID type, IDType hint = 0, bool ref = false, const wchar_t* stringhint = NULL)
		: Name(name), Type(type), Hint(hint), IsReference(ref), StringHint(stringhint)
	{ }

	const wchar_t* Name;
	VM::EpochVariableTypeID Type;
	IDType Hint;
	bool IsReference;
	const wchar_t* StringHint;
};

typedef void (__stdcall *RegisterFunctionFuncPtr)(const wchar_t* name, const char* internalname, const ParamData* params, size_t numparams, VM::EpochVariableTypeID returntype, void* bindrecord);
typedef void (__stdcall *RegisterConstantFuncPtr)(const wchar_t* name, VM::EpochVariableTypeID type, void* value, void* bindrecord);
typedef IDType (__stdcall *RegisterStructureFuncPtr)(const wchar_t* name, const ParamData* params, size_t numparams, void* bindrecord);
typedef void (__stdcall *RegisterExternalFuncPtr)(const wchar_t* name, const char* internalname, const wchar_t* dllname, const ParamData* params, size_t numparams, VM::EpochVariableTypeID returntype, void* bindrecord);
typedef void (__stdcall *RegisterSignatureFuncPtr)(const wchar_t* name, const ParamData* params, size_t numparams, VM::EpochVariableTypeID returntype, void* bindrecord);


//
// This structure provides all registration functions for the library to use
//
struct RegistrationTable
{
	RegisterFunctionFuncPtr RegisterFunction;
	RegisterConstantFuncPtr RegisterConstant;
	RegisterStructureFuncPtr RegisterStructure;
	RegisterExternalFuncPtr RegisterExternal;
	RegisterSignatureFuncPtr RegisterSignature;
};

