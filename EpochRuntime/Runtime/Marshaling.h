//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Declarations for marshaling for external code invocation and callbacks
//

#pragma once


// Forward declarations
class ActiveStructure;
class StructureDefinition;
class ScopeDescription;


// Dependencies
#include "Exports/ExportDef.h"
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"

namespace llvm
{
	class ExecutionEngine;
	class Function;
}

namespace Runtime
{

	//
	// Record for tracking the external library and function
	// that should be invoked by a given [external] tagged
	// Epoch function
	//
	struct DLLInvocationInfo
	{
		std::wstring DLLName;
		std::wstring FunctionName;
		std::wstring CallingConvention;
	};


	// Forward declarations
	class ExecutionContext;

	void RegisterMarshaledExternalFunction(StringHandle functionname, const std::wstring& dllname, const std::wstring& externalfunctionname, const std::wstring& callingconvention);
	const DLLInvocationInfo& GetMarshaledExternalFunction(StringHandle alias);
	bool IsMarshaledExternalFunction(StringHandle alias);

	EPOCHRUNTIME void MarshalBufferIntoStructureData(ExecutionContext& context, StructureHandle structure, const StructureDefinition& definition, const Byte* buffer);

	void PopulateWeakLinkages(const std::map<StringHandle, llvm::Function*>& externalfunctions, llvm::ExecutionEngine* ee);

	void MarshalIntoNativeCode(ExecutionContext& context, const ScopeDescription& scope, void* funcptr);


	void ResetMarshalingMetadata();

}


