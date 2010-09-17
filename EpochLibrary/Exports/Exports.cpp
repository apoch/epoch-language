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


namespace
{
	FunctionInvocationTable StandardLibraryFunctionDispatch;
}


extern "C" void __stdcall RegisterLibraryContents(FunctionSignatureSet& functionsignatures, StringPoolManager& stringpool)
{
	DebugLibrary::RegisterLibraryFunctions(functionsignatures, stringpool);
	DebugLibrary::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

	TypeConstructors::RegisterLibraryFunctions(functionsignatures, stringpool);
	TypeConstructors::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);

	TypeCasts::RegisterLibraryFunctions(functionsignatures, stringpool);
	TypeCasts::RegisterLibraryFunctions(StandardLibraryFunctionDispatch, stringpool);
}

extern "C" void __stdcall BindToVirtualMachine(FunctionInvocationTable& functiontable, StringPoolManager& stringpool)
{
	DebugLibrary::RegisterLibraryFunctions(functiontable, stringpool);
	TypeConstructors::RegisterLibraryFunctions(functiontable, stringpool);
	TypeCasts::RegisterLibraryFunctions(functiontable, stringpool);
}

extern "C" void __stdcall BindToCompiler(FunctionCompileHelperTable& functiontable, StringPoolManager& stringpool)
{
	DebugLibrary::RegisterLibraryFunctions(functiontable);
	TypeConstructors::RegisterLibraryFunctions(functiontable);
	TypeCasts::RegisterLibraryFunctions(functiontable);
}


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

