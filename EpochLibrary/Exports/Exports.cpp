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

#include "Virtual Machine/VirtualMachine.h"


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
	try
	{
		DebugLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
		DebugLibrary::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

		TypeConstructors::RegisterLibraryFunctions(functionsignatures, stringpool);
		TypeConstructors::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

		TypeCasts::RegisterLibraryFunctions(functionsignatures, stringpool);
		TypeCasts::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

		ArithmeticLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
		ArithmeticLibrary::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);
	}
	catch(...)
	{
		::MessageBox(0, L"Fatal error while registering Epoch standard library", L"Epoch Exception", MB_ICONSTOP);
	}
}

//
// Register the contents of this Epoch library with a virtual machine instance
//
// Strings are pooled in the VM's internal string pool, and functions
// are registered in the VM's global function dispatch table.
//
extern "C" void __stdcall BindToVirtualMachine(FunctionInvocationTable& functiontable, StringPoolManager& stringpool)
{
	try
	{
		DebugLibrary::RegisterLibraryFunctions(functiontable, stringpool);
		TypeConstructors::RegisterLibraryFunctions(functiontable, stringpool);
		TypeCasts::RegisterLibraryFunctions(functiontable, stringpool);
		ArithmeticLibrary::RegisterLibraryFunctions(functiontable, stringpool);
	}
	catch(...)
	{
		::MessageBox(0, L"Fatal error while registering Epoch standard library", L"Epoch Exception", MB_ICONSTOP);
	}
}

//
// Register the contents of this Epoch library with a compiler instance
//
// Strings are pooled in the compiler's internal string pool, syntax extensions
// are registered, and compile-time code helpers are bound.
//
extern "C" void __stdcall BindToCompiler(FunctionCompileHelperTable& functiontable, InfixTable& infixtable, StringPoolManager& stringpool, std::map<StringHandle, std::set<StringHandle> >& overloadmap)
{
	try
	{
		DebugLibrary::RegisterLibraryFunctions(functiontable);
		TypeConstructors::RegisterLibraryFunctions(functiontable);

		TypeCasts::RegisterLibraryFunctions(functiontable);
		TypeCasts::RegisterLibraryOverloads(overloadmap, stringpool);

		ArithmeticLibrary::RegisterLibraryFunctions(functiontable);
		ArithmeticLibrary::RegisterInfixOperators(infixtable, stringpool);
	}
	catch(...)
	{
		::MessageBox(0, L"Fatal error while registering Epoch standard library", L"Epoch Exception", MB_ICONSTOP);
	}
}


