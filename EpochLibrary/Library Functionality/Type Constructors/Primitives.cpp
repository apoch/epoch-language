//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for constructing variables of built-in primitive types
//

#include "pch.h"

#include "Library Functionality/Type Constructors/Primitives.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ScopeDescription.h"
#include "Metadata/ActiveScope.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/StringPool.h"


using namespace TypeConstructors;


//
// Bind the library to an execution dispatch table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	// TODO - complain on duplicates
	table.insert(std::make_pair(stringpool.Pool(L"integer"), TypeConstructors::ConstructInteger));
}

//
// Bind the library to a function metadata table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	// TODO - complain on duplicates
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier);
		signature.AddParameter(L"value", VM::EpochType_Integer);
		signatureset.insert(std::make_pair(stringpool.Pool(L"integer"), signature));
	}
}

//
// Bind the library to the compiler's internal semantic action table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	table.insert(std::make_pair(L"integer", TypeConstructors::CompileConstructorInteger));
}


void TypeConstructors::ConstructInteger(StringHandle functionname, VM::ExecutionContext& context)
{
	Integer32 value = context.State.Stack.PopValue<Integer32>();
	StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

	context.Variables->Write(identifierhandle, value);
}

void TypeConstructors::CompileConstructorInteger(ScopeDescription& scope, const std::vector<CompileTimeParameter>& compiletimeparams)
{
	scope.AddVariable(compiletimeparams[0].StringPayload, compiletimeparams[0].Payload.StringHandleValue, compiletimeparams[1].Type, VARIABLE_ORIGIN_LOCAL);
}

