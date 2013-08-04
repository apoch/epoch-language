//
// The Epoch Language Project
// Shared Library Code
//
// Central interface for retrieving information about types
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"


namespace Metadata
{
	size_t GetStorageSize(Metadata::EpochTypeID type);
	size_t GetMarshaledSize(Metadata::EpochTypeID type);
}
