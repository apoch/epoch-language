//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for debugging purposes
//

#include "pch.h"

#include "Library Functionality/Debugging/Debugging.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/StringPool.h"

#include "User Interface/Output.h"

#include "Virtual Machine/VirtualMachine.h"


using namespace DebugLibrary;


//
// Bind the library to an execution dispatch table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	// TODO - complain on duplicates
	table.insert(std::make_pair(stringpool.Pool(L"debugwritestring"), DebugLibrary::WriteString));
}

//
// Bind the library to a function metadata table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	// TODO - complain on duplicates
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", VM::EpochType_String);
		signatureset.insert(std::make_pair(stringpool.Pool(L"debugwritestring"), signature));
	}
}

//
// Bind the library to the compiler's internal semantic action table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	// Nothing to do for this library
}


//
// Write a string to the debug output
//
void DebugLibrary::WriteString(StringHandle functionname, VM::ExecutionContext& context)
{
	StringHandle handle = context.State.Stack.PopValue<StringHandle>();

	UI::OutputStream stream;
	stream << UI::lightblue << L"DEBUG: " << UI::resetcolor << context.OwnerVM.GetPooledString(handle) << std::endl;
}

