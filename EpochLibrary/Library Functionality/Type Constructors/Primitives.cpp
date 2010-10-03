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
#include "Utility/NoDupeMap.h"


using namespace TypeConstructors;


//
// Bind the library to an execution dispatch table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"integer"), TypeConstructors::ConstructInteger));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"string"), TypeConstructors::ConstructString));
}

//
// Bind the library to a function metadata table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier);
		signature.AddParameter(L"value", VM::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier);
		signature.AddParameter(L"value", VM::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"string"), signature));
	}
}

//
// Bind the library to the compiler's internal semantic action table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"integer", TypeConstructors::CompileConstructorPrimitive));
	AddToMapNoDupe(table, std::make_pair(L"string", TypeConstructors::CompileConstructorPrimitive));
}


//
// Construct an integer variable in memory
//
void TypeConstructors::ConstructInteger(StringHandle functionname, VM::ExecutionContext& context)
{
	Integer32 value = context.State.Stack.PopValue<Integer32>();
	StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

	context.Variables->Write(identifierhandle, value);
}

//
// Construct a string variable in memory
//
void TypeConstructors::ConstructString(StringHandle functionname, VM::ExecutionContext& context)
{
	StringHandle value = context.State.Stack.PopValue<StringHandle>();
	StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

	context.Variables->Write(identifierhandle, value);
}


//
// Compile-time helper: when a variable definition is encountered, this
// helper adds the variable itself and its type metadata to the current
// lexical scope.
//
void TypeConstructors::CompileConstructorPrimitive(ScopeDescription& scope, const std::vector<CompileTimeParameter>& compiletimeparams)
{
	VM::EpochTypeID effectivetype = compiletimeparams[1].Type;
	if(effectivetype == VM::EpochType_Identifier)
		effectivetype = scope.GetVariableTypeByID(compiletimeparams[1].Payload.StringHandleValue);
	else if(effectivetype == VM::EpochType_Expression)
		effectivetype = compiletimeparams[1].ExpressionType;
	scope.AddVariable(compiletimeparams[0].StringPayload, compiletimeparams[0].Payload.StringHandleValue, effectivetype, VARIABLE_ORIGIN_LOCAL);
}
