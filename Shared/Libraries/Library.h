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
#include "Metadata/IdentifierTable.h"
#include "Utility/Types/IDTypes.h"

#include <map>


// Forward declarations
namespace IRSemantics
{
	class Statement;
	class CodeBlock;
	class Program;
}

namespace VM
{
	class ExecutionContext;
}


// Handy type shortcuts
typedef void (*EpochFunctionPtr)(StringHandle namehandle, VM::ExecutionContext& context);
typedef std::map<StringHandle, EpochFunctionPtr> FunctionInvocationTable;

typedef void (*CompilerHelperPtr)(IRSemantics::Statement& statement, IRSemantics::Program& program, IRSemantics::CodeBlock& activescope, bool inreturnexpr);
typedef std::map<StringHandle, CompilerHelperPtr> FunctionCompileHelperTable;

typedef std::multimap<int, StringHandle> PrecedenceTable;

typedef std::map<Bytecode::EntityTag, EntityDescription> EntityTable;

typedef std::set<StringHandle> StringHandleSet;

typedef std::map<StringHandle, StringHandleSet> OverloadMap;


namespace JIT
{
	struct JITTable;
}


//
// This structure encapsulates the data associated with function tags, allowing
// the tag parser helper to attach metadata to a function for storage in final
// compiled bytecode. It also allows runtime functions to be invoked magically
// when the function is called, which provides a handy mechanism for marshaling
// calls to external APIs.
//
struct TagHelperReturn
{
	TagHelperReturn()
		: LinkToCompileTimeHelper(NULL),
		  SetConstructorFunction(0)
	{
	}

	std::wstring InvokeRuntimeFunction;
	std::wstring MetaTag;
	std::vector<std::wstring> MetaTagData;

	CompilerHelperPtr LinkToCompileTimeHelper;
	StringHandle SetConstructorFunction;
};


typedef TagHelperReturn (*FunctionTagHelperPtr)(StringHandle functionname, const CompileTimeParameterVector& compiletimeparams, bool isprepass);
typedef std::map<std::wstring, FunctionTagHelperPtr> FunctionTagHelperTable;


//
// Convenience structure holding a set of information needed for compilation
//
// Note that in most cases this will consist purely of pointers bound to another
// object, such as a CompileSession, which stores all the data indirectly. This
// is mainly useful for providing a succinct interface for passing the various
// pointers into external library DLLs.
//
struct CompilerInfoTable
{
	FunctionCompileHelperTable* FunctionHelpers;
	StringSet* InfixOperators;
	StringSet* UnaryPrefixes;
	StringSet* PreOperators;
	StringSet* PostOperators;
	StringSet* OpAssignOperators;
	PrecedenceTable* Precedences;
	OverloadMap* Overloads;
	EntityTable* Entities;
	EntityTable* ChainedEntities;
	EntityTable* PostfixEntities;
	EntityTable* PostfixClosers;
	FunctionTagHelperTable* FunctionTagHelpers;
};

