//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once
#include <boost/config.hpp>


// Standard C++ Library headers
#include <string>
#include <sstream>
#include <set>


// Platform-specific stuff
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501		// Windows XP or later
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#define STDCALL __stdcall

// Epoch project standard shared code
#include "Utility/Exception.h"


// Leak checking
//#define VLD_FORCE_ENABLE
//#include <vld.h>
