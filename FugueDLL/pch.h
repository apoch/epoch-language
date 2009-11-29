//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once

// Standard C++ Library headers
#include <string>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <stack>
#include <sstream>
#include <set>
#include <algorithm>
#include <locale>


// Platform-specific stuff
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501		// Windows XP or later
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>


// Assorted type definitions
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/IDTypes.h"


// String lookups
#include "String Data/StringConstants.h"
#include "String Data/Keywords.h"
#include "String Data/Operators.h"


// Our own utility code
#include "Utility/Exception.h"


