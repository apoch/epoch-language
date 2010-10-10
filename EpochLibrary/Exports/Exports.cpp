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
#include "Library Functionality/Operators/Boolean.h"
#include "Library Functionality/Operators/Comparison.h"
#include "Library Functionality/Operators/Strings.h"

#include "Library Functionality/Flow Control/Conditionals.h"
#include "Library Functionality/Flow Control/Loops.h"
#include "Library Functionality/Flow Control/StringPooling.h"

#include "Virtual Machine/VirtualMachine.h"


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
		TypeConstructors::RegisterLibraryFunctions(functionsignatures, stringpool);
		TypeCasts::RegisterLibraryFunctions(functionsignatures, stringpool);
		ArithmeticLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
		BooleanLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
		ComparisonLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
		StringLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
		FlowControl::RegisterStrings(stringpool);
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
extern "C" void __stdcall BindToVirtualMachine(FunctionInvocationTable& functiontable, EntityTable& entities, EntityTable& chainedentities, StringPoolManager& stringpool, Bytecode::EntityTag& tagindex)
{
	try
	{
		DebugLibrary::RegisterLibraryFunctions(functiontable, stringpool);
		TypeConstructors::RegisterLibraryFunctions(functiontable, stringpool);
		TypeCasts::RegisterLibraryFunctions(functiontable, stringpool);
		ArithmeticLibrary::RegisterLibraryFunctions(functiontable, stringpool);
		BooleanLibrary::RegisterLibraryFunctions(functiontable, stringpool);
		ComparisonLibrary::RegisterLibraryFunctions(functiontable, stringpool);
		StringLibrary::RegisterLibraryFunctions(functiontable, stringpool);

		FlowControl::RegisterConditionalEntities(entities, entities, stringpool, tagindex);
		FlowControl::RegisterLoopEntities(entities, entities, entities, entities, stringpool, tagindex);
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
extern "C" void __stdcall BindToCompiler(CompilerInfoTable& info, StringPoolManager& stringpool, Bytecode::EntityTag& tagindex)
{
	try
	{
		TypeConstructors::RegisterLibraryFunctions(*info.FunctionHelpers);

		TypeCasts::RegisterLibraryOverloads(*info.Overloads, stringpool);

		ArithmeticLibrary::RegisterInfixOperators(*info.InfixOperators, *info.Precedences, stringpool);
		ArithmeticLibrary::RegisterUnaryOperators(*info.UnaryPrefixes, stringpool);
		ArithmeticLibrary::RegisterOpAssignOperators(*info.OpAssignOperators, stringpool);
		ArithmeticLibrary::RegisterLibraryOverloads(*info.Overloads, stringpool);

		BooleanLibrary::RegisterLibraryOverloads(*info.Overloads, stringpool);

		ComparisonLibrary::RegisterInfixOperators(*info.InfixOperators, *info.Precedences, stringpool);

		StringLibrary::RegisterInfixOperators(*info.InfixOperators, *info.Precedences, stringpool);

		FlowControl::RegisterConditionalEntities(*info.Entities, *info.ChainedEntities, stringpool, tagindex);
		FlowControl::RegisterLoopEntities(*info.Entities, *info.ChainedEntities, *info.PostfixEntities, *info.PostfixClosers, stringpool, tagindex);
	}
	catch(...)
	{
		::MessageBox(0, L"Fatal error while registering Epoch standard library", L"Epoch Exception", MB_ICONSTOP);
	}
}



