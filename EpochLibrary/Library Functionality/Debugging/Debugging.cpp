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
#include "Utility/NoDupeMap.h"

#include "User Interface/Output.h"
#include "User Interface/Input.h"

#include "Virtual Machine/VirtualMachine.h"


using namespace DebugLibrary;


//
// Bind the library to an execution dispatch table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"debugwritestring"), DebugLibrary::WriteString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"debugreadstring"), DebugLibrary::ReadString));
}

//
// Bind the library to a function metadata table
//
void DebugLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"str", VM::EpochType_String, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"debugwritestring"), signature));
	}
	{
		FunctionSignature signature;
		signature.SetReturnType(VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"debugreadstring"), signature));
	}
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

//
// Read a string from the debug console
//
void DebugLibrary::ReadString(StringHandle functionname, VM::ExecutionContext& context)
{
	UI::Input input;
	StringHandle handle = context.OwnerVM.PoolString(input.BlockingRead());
	context.State.Stack.PushValue(handle);
}

