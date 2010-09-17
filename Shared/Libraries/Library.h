//
// The Epoch Language Project
// Shared Library Code
//
// Interface for defining built-in library functions
//

#pragma once


// Dependencies
#include "Metadata/CompileTimeParams.h"
#include "Utility/Types/IDTypes.h"

#include <map>
#include <string>
#include <vector>


// Forward declarations
class ScopeDescription;

namespace VM
{
	class ExecutionContext;
}


// Handy type shortcuts
typedef void (*EpochFunctionPtr)(StringHandle namehandle, VM::ExecutionContext& context);
typedef std::map<StringHandle, EpochFunctionPtr> FunctionInvocationTable;

typedef void (*CompilerHelperPtr)(ScopeDescription& scope, const std::vector<CompileTimeParameter>& compiletimeparams);
typedef std::map<std::wstring, CompilerHelperPtr> FunctionCompileHelperTable;

