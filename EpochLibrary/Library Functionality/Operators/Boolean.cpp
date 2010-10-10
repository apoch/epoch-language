//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for boolean operators
//

#include "pch.h"

#include "Library Functionality/Operators/Boolean.h"
#include "Library Functionality/Operators/Precedences.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Libraries/Library.h"

#include "Virtual Machine/VirtualMachine.h"



namespace
{

	//
	// Compute the logical negation of an operand
	//
	void BooleanNot(StringHandle functionname, VM::ExecutionContext& context)
	{
		bool p = context.State.Stack.PopValue<bool>();
		context.State.Stack.PushValue(!p);
	}

}



//
// Bind the library to an execution dispatch table
//
void BooleanLibrary::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"!@@boolean"), BooleanNot));
}

//
// Bind the library to a function metadata table
//
void BooleanLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"b", VM::EpochType_Boolean);
		signature.SetReturnType(VM::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"!@@boolean"), signature));
	}
}

//
// Register the list of overloads used by functions in this library module
//
void BooleanLibrary::RegisterLibraryOverloads(std::map<StringHandle, std::set<StringHandle> >& overloadmap, StringPoolManager& stringpool)
{
	{
		StringHandle functionnamehandle = stringpool.Pool(L"!");
		overloadmap[functionnamehandle].insert(stringpool.Pool(L"!@@boolean"));
	}
}

