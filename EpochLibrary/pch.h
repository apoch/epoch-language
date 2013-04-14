//
// The Epoch Language Project
// Epoch Standard Library
//
// Precompiled header - commonly used headers etc.
// All code modules should include this header.
//

#pragma once
#include <boost/config.hpp>


// Platform-specific stuff
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501		// Windows XP or later
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#define STDCALL __stdcall


#include <boost/range/iterator_range.hpp>
#include <boost/unordered_set.hpp>


#pragma warning(push)
#pragma warning(disable: 4996)		// unsafe usage of std::copy


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
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IRBuilder.h"
#include "llvm/Attributes.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/DefaultPasses.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ExecutionEngine/JITEventListener.h"

#pragma warning(pop)
#pragma warning(pop)


// Epoch options
#define EPOCH_STRINGPOOL_FAST_REVERSE_LOOKUP

// Epoch project standard code
#include "Utility/Exception.h"

