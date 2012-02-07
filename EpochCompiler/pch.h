//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once
#pragma warning(disable : 4503)

// Standard C++ library stuff
#include <string>
#include <map>
#include <set>
#include <vector>
#include <list>


// Platform-specific stuff
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>


// Epoch project standard code
#include "Utility/Exception.h"


// Options for Boost
#define BOOST_EXECPTION_DISABLE
#define BOOST_SPIRIT_UNICODE
#define BOOST_SPIRIT_DONT_USE_MPL_ASSERT_MSG 1		// Avoid nasty linker glitch in mpl


// We include spirit here for better grammar/parser build times
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_match_auto.hpp>
#include <boost/spirit/include/qi_symbols.hpp>

#include <boost/spirit/include/lex_lexertl.hpp>

#include <boost/variant.hpp>

#include <boost/fusion/adapted.hpp>

#include <boost/optional.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
