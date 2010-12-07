//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Declarations for marshaling for external code invocation and callbacks
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"


namespace VM
{

	// Forward declarations
	class ExecutionContext;

	void RegisterMarshaledExternalFunction(StringHandle functionname, const std::wstring& dllname, const std::wstring& externalfunctionname);

}


