//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Declarations for marshaling for external code invocation and callbacks
//

#pragma once


// Forward declarations
class ActiveStructure;
class StructureDefinition;
class ScopeDescription;


// Dependencies
#include "Virtual Machine/ExportDef.h"
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"


namespace VM
{

	// Forward declarations
	class ExecutionContext;

	void RegisterMarshaledExternalFunction(StringHandle functionname, const std::wstring& dllname, const std::wstring& externalfunctionname);

	EPOCHVM void MarshalBufferIntoStructureData(ExecutionContext& context, ActiveStructure& structure, const StructureDefinition& definition, const Byte* buffer);


	void MarshalIntoNativeCode(ExecutionContext& context, const ScopeDescription& scope, void* funcptr);

}


