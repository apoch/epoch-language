//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once


// Standard C++ library stuff
#include <string>
#include <map>
#include <set>
#include <vector>


// Platform-specific stuff
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>


// Epoch project standard code
#include "Utility/Exception.h"



// Options for Boost
#define BOOST_EXECPTION_DISABLE
#define BOOST_SPIRIT_UNICODE

// We include spirit here for better grammar/parser build times
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_match_auto.hpp>
