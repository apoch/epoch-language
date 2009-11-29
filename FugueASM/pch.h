//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once

#include <string>
#include <sstream>
#include <iostream>

// Platform-specific stuff
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501		// Windows XP or later
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#include "Utility/Exception.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

