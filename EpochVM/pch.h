//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once
#include <boost/config.hpp>

#ifdef BOOST_WINDOWS
#ifdef NDEBUG
#define _SECURE_SCL 0
#endif
#endif

#pragma warning(push)
#pragma warning(disable: 4996)		// unsafe usage of std::copy

// Standard C++ library stuff
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>


#include <boost/unordered_set.hpp>


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


// LLVM
#pragma warning(push)
#pragma warning(disable: 4800)		// coercion to bool
#pragma warning(disable: 4146)		// unary minus on unsigned
#pragma warning(disable: 4355)		// "this" in initializer list
#pragma warning(disable: 4512)		// cannot generate assignment operator
#pragma warning(disable: 4127)		// conditional expression is constant
#pragma warning(disable: 4245)		// signed/unsigned mismatch
#pragma warning(disable: 4244)		// conversion might lose data
#pragma warning(disable: 4100)		// unreferenced formal parameter

#define SUPPORT_DATATYPES_H 1
#define END_WITH_NULL

#include <stdint.h>

#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/DefaultPasses.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/ADT/SmallVector.h"

#pragma warning(pop)
#pragma warning(pop)
