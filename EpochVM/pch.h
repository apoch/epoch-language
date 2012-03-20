//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once
#include <boost/config.hpp>

// Standard C++ library stuff
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>


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


#define STATIC_ASSERT(expr)			enum { dummy = 1/static_cast<int>(!!(expr)) }