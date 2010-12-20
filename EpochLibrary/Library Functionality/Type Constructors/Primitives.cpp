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

#include "Compilation/SemanticActionInterface.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


namespace
{

	//
	// Construct an integer variable in memory
	//
	void ConstructInteger(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 value = context.State.Stack.PopValue<Integer32>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct an integer16 variable in memory
	//
	void ConstructInteger16(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer16 value = context.State.Stack.PopValue<Integer16>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct a string variable in memory
	//
	void ConstructString(StringHandle functionname, VM::ExecutionContext& context)
	{
		StringHandle value = context.State.Stack.PopValue<StringHandle>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct a boolean variable in memory
	//
	void ConstructBoolean(StringHandle functionname, VM::ExecutionContext& context)
	{
		bool value = context.State.Stack.PopValue<bool>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct a real variable in memory
	//
	void ConstructReal(StringHandle functionname, VM::ExecutionContext& context)
	{
		Real32 value = context.State.Stack.PopValue<Real32>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct a buffer in memory
	//
	// Note that unlike primitives which can be constructed and initialized from default
	// literal values in the code, buffers cannot be specified as literals; this means
	// that the constructor does not accept a literal buffer argument, but instead a
	// size value in characters (not bytes!) which the buffer should allocate space for.
	// Since this constructor allocates memory in the form of a buffer, it should tick
	// over the garbage collector.
	//
	void ConstructBuffer(StringHandle functionname, VM::ExecutionContext& context)
	{
		Integer32 size = context.State.Stack.PopValue<Integer32>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		BufferHandle bufferhandle = context.OwnerVM.AllocateBuffer((size + 1) * sizeof(wchar_t));
		context.Variables->Write(identifierhandle, bufferhandle);
		context.TickBufferGarbageCollector();
	}


	//
	// Compile-time helper: when a variable definition is encountered, this
	// helper adds the variable itself and its type metadata to the current
	// lexical scope.
	//
	void CompileConstructorPrimitive(const std::wstring& functionname, SemanticActionInterface& semantics, ScopeDescription& scope, const CompileTimeParameterVector& compiletimeparams)
	{
		VM::EpochTypeID effectivetype = semantics.LookupTypeName(functionname);
		scope.AddVariable(compiletimeparams[0].StringPayload, compiletimeparams[0].Payload.StringHandleValue, effectivetype, false, VARIABLE_ORIGIN_LOCAL);
	}

}


//
// Bind the library to an execution dispatch table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"integer"), ConstructInteger));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"integer16"), ConstructInteger16));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"string"), ConstructString));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"boolean"), ConstructBoolean));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"real"), ConstructReal));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"buffer"), ConstructBuffer));
}

//
// Bind the library to a function metadata table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"value", VM::EpochType_Integer, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"value", VM::EpochType_Integer16, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"integer16"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"value", VM::EpochType_String, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"value", VM::EpochType_Boolean, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"boolean"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"value", VM::EpochType_Real, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, false);
		signature.AddParameter(L"size", VM::EpochType_Integer, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"buffer"), signature));
	}
}

//
// Bind the library to the compiler's internal semantic action table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"integer", CompileConstructorPrimitive));
	AddToMapNoDupe(table, std::make_pair(L"integer16", CompileConstructorPrimitive));
	AddToMapNoDupe(table, std::make_pair(L"string", CompileConstructorPrimitive));
	AddToMapNoDupe(table, std::make_pair(L"boolean", CompileConstructorPrimitive));
	AddToMapNoDupe(table, std::make_pair(L"real", CompileConstructorPrimitive));
	AddToMapNoDupe(table, std::make_pair(L"buffer", CompileConstructorPrimitive));
}


