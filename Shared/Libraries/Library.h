//
// The Epoch Language Project
// Shared Library Code
//
// Interface for defining built-in library functions
//

#pragma once


// Dependencies
#include "Metadata/EntityDescription.h"
#include "Metadata/CompileTimeParams.h"
#include "Utility/Types/IDTypes.h"

#include <map>
#include <set>
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

typedef std::set<std::wstring> InfixTable;
typedef std::multimap<int, StringHandle> PrecedenceTable;

typedef std::map<Bytecode::EntityTag, EntityDescription> EntityTable;


struct CompilerInfoTable
{
	FunctionCompileHelperTable* FunctionHelpers;
	InfixTable* InfixOperators;
	std::set<std::wstring>* UnaryPrefixes;
	PrecedenceTable* Precedences;
	std::map<StringHandle, std::set<StringHandle> >* Overloads;
	EntityTable* Entities;
	EntityTable* ChainedEntities;
	EntityTable* PostfixEntities;
	EntityTable* PostfixClosers;
};

