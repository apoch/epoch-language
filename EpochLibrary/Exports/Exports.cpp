//
// The Epoch Language Project
// Epoch Standard Library
//
// Interface for integrating standard library functionality into the compiler and VM
//

#include "pch.h"

#include "Library Functionality/Debugging/Debugging.h"
#include "Library Functionality/Type Constructors/Primitives.h"
#include "Library Functionality/Type Casting/Typecasts.h"
#include "Library Functionality/Operators/Arithmetic.h"


// Internal data tracking
namespace
{
	FunctionInvocationTable StandardLibraryFunctionDispatch;
}


//
// Register the contents of this Epoch library
//
// This process includes setting up the function signatures and pooling string
// identifiers for all library entities, types, constants, and so on.
//
extern "C" void __stdcall RegisterLibraryContents(FunctionSignatureSet& functionsignatures, StringPoolManager& stringpool)
{
	// TODO - exception wrappers around ALL export functions (here and in other DLLs)
	DebugLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
	DebugLibrary::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

	TypeConstructors::RegisterLibraryFunctions(functionsignatures, stringpool);
	TypeConstructors::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

	TypeCasts::RegisterLibraryFunctions(functionsignatures, stringpool);
	TypeCasts::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

	ArithmeticLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
	ArithmeticLibrary::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);
}

//
// Register the contents of this Epoch library with a virtual machine instance
//
// Strings are pooled in the VM's internal string pool, and functions
// are registered in the VM's global function dispatch table.
//
extern "C" void __stdcall BindToVirtualMachine(FunctionInvocationTable& functiontable, StringPoolManager& stringpool)
{
	DebugLibrary::RegisterLibraryFunctions(functiontable, stringpool);
	TypeConstructors::RegisterLibraryFunctions(functiontable, stringpool);
	TypeCasts::RegisterLibraryFunctions(functiontable, stringpool);
	ArithmeticLibrary::RegisterLibraryFunctions(functiontable, stringpool);
}

//
// Register the contents of this Epoch library with a compiler instance
//
// Strings are pooled in the compiler's internal string pool, syntax extensions
// are registered, and compile-time code helpers are bound.
//
extern "C" void __stdcall BindToCompiler(FunctionCompileHelperTable& functiontable, InfixTable& infixtable, StringPoolManager& stringpool, std::map<StringHandle, std::set<StringHandle> >& overloadmap)
{
	DebugLibrary::RegisterLibraryFunctions(functiontable);
	TypeConstructors::RegisterLibraryFunctions(functiontable);

	TypeCasts::RegisterLibraryFunctions(functiontable);
	TypeCasts::RegisterLibraryOverloads(overloadmap, stringpool);

	ArithmeticLibrary::RegisterLibraryFunctions(functiontable);
	ArithmeticLibrary::RegisterInfixOperators(infixtable, stringpool);
}

//
// Invoke a function hosted by this library
//
extern "C" void __stdcall InvokeLibraryFunction(StringHandle functionname, VM::ExecutionContext& context)
{
	FunctionInvocationTable::const_iterator iter = StandardLibraryFunctionDispatch.find(functionname);
	if(iter == StandardLibraryFunctionDispatch.end())
	{
		// TODO - flag an error in the execution context
		return;
	}

	iter->second(functionname, context);
}

