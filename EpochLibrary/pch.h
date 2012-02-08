//
// The Epoch Language Project
// Epoch Standard Library
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once


// Platform-specific stuff
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501		// Windows XP or later
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#define STDCALL __stdcall

// Epoch project standard code
#include "Utility/Exception.h"

