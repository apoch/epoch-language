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

#include "Library Functionality/Function Tags/Externals.h"
#include "Library Functionality/Function Tags/Constructors.h"

#include "Library Functionality/Marshaling/MarshalingLibrary.h"

#include "Library Functionality/Command Line/CommandLine.h"

#include "Library Functionality/Strings/Strings.h"

#include "Runtime/Runtime.h"


namespace
{
	void PoolAllStrings(StringPoolManager& stringpool)
	{
		ArithmeticLibrary::PoolStrings(stringpool);
		BooleanLibrary::PoolStrings(stringpool);
		CommandLineLibrary::PoolStrings(stringpool);
		ComparisonLibrary::PoolStrings(stringpool);
		DebugLibrary::PoolStrings(stringpool);
		FlowControl::PoolStrings(stringpool);
		FunctionTags::PoolStrings(stringpool);
		MarshalingLibrary::PoolStrings(stringpool);
		StringFunctionLibrary::PoolStrings(stringpool);
		StringLibrary::PoolStrings(stringpool);
		TypeCasts::PoolStrings(stringpool);
		TypeConstructors::PoolStrings(stringpool);
	}
}


Runtime::ExecutionContext* GlobalExecutionContext = NULL;


//
// Register the contents of this Epoch library
//
// This process includes setting up the function signatures and pooling string
// identifiers for all library entities, types, constants, and so on.
//
extern "C" void STDCALL RegisterLibraryContents(FunctionSignatureSet& functionsignatures, StringPoolManager& stringpool)
{
	try
	{
		PoolAllStrings(stringpool);

		DebugLibrary::RegisterLibraryFunctions(functionsignatures);
		TypeConstructors::RegisterLibraryFunctions(functionsignatures);
		TypeCasts::RegisterLibraryFunctions(functionsignatures);
		ArithmeticLibrary::RegisterLibraryFunctions(functionsignatures);
		BooleanLibrary::RegisterLibraryFunctions(functionsignatures);
		ComparisonLibrary::RegisterLibraryFunctions(functionsignatures);
		StringLibrary::RegisterLibraryFunctions(functionsignatures);

		FunctionTags::RegisterExternalTag(functionsignatures);

		MarshalingLibrary::RegisterLibraryFunctions(functionsignatures);

		CommandLineLibrary::RegisterLibraryFunctions(functionsignatures);
		StringFunctionLibrary::RegisterLibraryFunctions(functionsignatures);
	}
	catch(...)
	{
		::MessageBox(0, L"Fatal error while registering Epoch standard library contents", L"Epoch Initialization Exception", MB_ICONSTOP);
	}
}

//
// Register the contents of this Epoch library with a runtime instance
//
// Strings are pooled in the VM's internal string pool, and functions
// are registered in the VM's global function dispatch table.
//
extern "C" void STDCALL BindToRuntime(Runtime::ExecutionContext* context, StringPoolManager& stringpool, JIT::JITTable& jittable)
{
	try
	{
		GlobalExecutionContext = context;

		PoolAllStrings(stringpool);

		Bytecode::EntityTag tagindex = Bytecode::EntityTags::CustomEntityBaseID;		// TODO - this might bite us later. Consider pulling from the context.
		FlowControl::RegisterConditionalEntitiesJIT(tagindex);
		FlowControl::RegisterLoopEntitiesJIT(tagindex);

		ArithmeticLibrary::RegisterJITTable(jittable);
		BooleanLibrary::RegisterJITTable(jittable);
		ComparisonLibrary::RegisterJITTable(jittable);
		DebugLibrary::RegisterJITTable(jittable);
		FlowControl::RegisterConditionalJITTable(jittable);
		FlowControl::RegisterLoopsJITTable(jittable);
		FunctionTags::RegisterExternalTagJITTable(jittable);
		MarshalingLibrary::RegisterJITTable(jittable);
		StringFunctionLibrary::RegisterJITTable(jittable);
		StringLibrary::RegisterJITTable(jittable);
		TypeCasts::RegisterJITTable(jittable);
		TypeConstructors::RegisterJITTable(jittable);
	}
	catch(...)
	{
		::MessageBox(0, L"Fatal error while registering Epoch standard library with runtime", L"Epoch Initialization Exception", MB_ICONSTOP);
	}
}

//
// Register the contents of this Epoch library with a compiler instance
//
// Strings are pooled in the compiler's internal string pool, syntax extensions
// are registered, and compile-time code helpers are bound.
//
extern "C" void STDCALL BindToCompiler(CompilerInfoTable& info, StringPoolManager& stringpool, Bytecode::EntityTag& tagindex)
{
	try
	{
		PoolAllStrings(stringpool);

		TypeConstructors::RegisterLibraryFunctions(*info.FunctionHelpers);

		TypeCasts::RegisterLibraryOverloads(*info.Overloads, stringpool);

		ArithmeticLibrary::RegisterInfixOperators(*info.InfixOperators, *info.Precedences);
		ArithmeticLibrary::RegisterUnaryOperators(*info.UnaryPrefixes, *info.PreOperators, *info.PostOperators);
		ArithmeticLibrary::RegisterOpAssignOperators(*info.OpAssignOperators);
		ArithmeticLibrary::RegisterLibraryOverloads(*info.Overloads);

		BooleanLibrary::RegisterLibraryOverloads(*info.Overloads);

		ComparisonLibrary::RegisterInfixOperators(*info.InfixOperators, *info.Precedences);
		ComparisonLibrary::RegisterLibraryOverloads(*info.Overloads);

		StringLibrary::RegisterInfixOperators(*info.InfixOperators, *info.Precedences);

		FlowControl::RegisterConditionalEntities(*info.Entities, *info.ChainedEntities, tagindex);
		FlowControl::RegisterLoopEntities(*info.Entities, *info.ChainedEntities, *info.PostfixEntities, *info.PostfixClosers, tagindex);

		FunctionTags::RegisterExternalTagHelper(*info.FunctionTagHelpers);
		FunctionTags::RegisterConstructorTagHelper(*info.FunctionTagHelpers);

		StringFunctionLibrary::RegisterLibraryOverloads(*info.Overloads);
	}
	catch(...)
	{
		::MessageBox(0, L"Fatal error while registering Epoch standard library with compiler", L"Epoch Initialization Exception", MB_ICONSTOP);
	}
}



extern "C" void STDCALL LinkToTestHarness(unsigned* harness)
{
	//try
	{
		DebugLibrary::LinkToTestHarness(harness);
	}
	//catch(...)
	//{
	//	::MessageBox(0, L"Fatal error while registering Epoch standard test harness", L"Epoch Initialization Exception", MB_ICONSTOP);
	//}
}

