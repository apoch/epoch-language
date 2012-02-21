//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Central interface for retrieving information about types
//

#pragma once


// Dependencies
#include "Virtual Machine/ExportDef.h"
#include "Utility/Types/EpochTypeIDs.h"


namespace VM
{
	EPOCHVM size_t GetStorageSize(VM::EpochTypeID type);
	EPOCHVM size_t GetMarshaledSize(VM::EpochTypeID type);
}
