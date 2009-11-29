//
// The Epoch Language Project
// Win32 EXE Generator
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <stack>


// Platform-specific stuff
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501		// Windows XP or later
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>


#include "Utility/Types/IntegerTypes.h"
#include "Utility/Exception.h"
