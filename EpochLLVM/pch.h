//
// The Epoch Language Project
// Epoch Development Tools - LLVM wrapper library
//
// PCH.H
// Precompiled header
//


#pragma once


#pragma warning(push)
#pragma warning(disable: 4996)		// unsafe functions


// Target latest Windows SDK version
#include <SDKDDKVer.h>


// Windows header options
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Windows headers
#include <windows.h>


// C++ standard headers
#include <vector>
#include <iostream>


// LLVM headers
#pragma warning(push)
#pragma warning(disable: 4800)		// coercion to bool
#pragma warning(disable: 4146)		// unary minus on unsigned
#pragma warning(disable: 4355)		// "this" in initializer list
#pragma warning(disable: 4512)		// cannot generate assignment operator
#pragma warning(disable: 4127)		// conditional expression is constant
#pragma warning(disable: 4245)		// signed/unsigned mismatch
#pragma warning(disable: 4244)		// conversion might lose data
#pragma warning(disable: 4100)		// unreferenced formal parameter
#pragma warning(disable: 4624)		// cannot generate destructor

#define SUPPORT_DATATYPES_H 1
#define END_WITH_NULL

#include <stdint.h>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
//#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Attributes.h"
#include <llvm/IR/Verifier.h>
#include "llvm/Support/TargetSelect.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/ADT/SmallVector.h"
//#include "llvm/ExecutionEngine/JITMemoryManager.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include <llvm/Object/ObjectFile.h>
//#include <llvm/Support/system_error.h>
#include <llvm/ADT/Triple.h>

#pragma warning(pop)


#pragma warning(pop)

