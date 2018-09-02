// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#pragma warning(push)
#pragma warning(disable: 4996)		// unsafe functions


// TODO: reference additional headers your program requires here
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <limits>
#include <sstream>

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
#pragma warning(disable: 4141)		// inline used more than once
#pragma warning(disable: 4291)		// no matching operator delete


#define END_WITH_NULL

#include <stdint.h>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/RtDyldMemoryManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/ADT/Triple.h>
#include <llvm/CodeGen/GCStrategy.h>
#include <llvm/CodeGen/GCMetadata.h>
#include <llvm/Support/Compiler.h>
#include <llvm/CodeGen/GCMetadataPrinter.h>
#include <llvm/CodeGen/AsmPrinter.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/MCSymbol.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/Support/Memory.h>

#pragma warning(pop)

#pragma warning(pop)


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
// Windows Header Files:
#include <windows.h>
