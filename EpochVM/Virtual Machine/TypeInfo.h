//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Central interface for retrieving information about types
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"


namespace VM
{
	size_t GetStorageSize(VM::EpochTypeID type);
	size_t GetMarshaledSize(VM::EpochTypeID type);
}
