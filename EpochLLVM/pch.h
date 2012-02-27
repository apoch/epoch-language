#pragma once

#include <tchar.h>


#pragma warning(push)
#pragma warning(disable: 4800)		// coercion to bool
#pragma warning(disable: 4146)		// unary minus on unsigned
#pragma warning(disable: 4355)		// "this" in initializer list
#pragma warning(disable: 4996)		// unsafe usage of std::copy

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

#pragma warning(pop)

#include <vector>
#include <iostream>

