//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for constructing variables of built-in primitive types
//

#include "pch.h"

#include "Library Functionality/Type Constructors/Primitives.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/CompileErrors.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Libraries/LibraryJIT.h"

#include "Metadata/ScopeDescription.h"
#include "Metadata/ActiveScope.h"

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
	void ConstructInteger(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 value = context.State.Stack.PopValue<Integer32>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	void ConstructIntegerJIT(JIT::JITContext& context)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateStore(p2, context.VariableMap[context.NameToIndexMap[vartarget]], false);
	}

	//
	// Construct an integer16 variable in memory
	//
	void ConstructInteger16(StringHandle, VM::ExecutionContext& context)
	{
		Integer16 value = context.State.Stack.PopValue<Integer16>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct a string variable in memory
	//
	void ConstructString(StringHandle, VM::ExecutionContext& context)
	{
		StringHandle value = context.State.Stack.PopValue<StringHandle>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct a boolean variable in memory
	//
	void ConstructBoolean(StringHandle, VM::ExecutionContext& context)
	{
		bool value = context.State.Stack.PopValue<bool>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	//
	// Construct a real variable in memory
	//
	void ConstructReal(StringHandle, VM::ExecutionContext& context)
	{
		Real32 value = context.State.Stack.PopValue<Real32>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		context.Variables->Write(identifierhandle, value);
	}

	void ConstructRealJIT(JIT::JITContext& context)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateStore(p2, context.VariableMap[context.NameToIndexMap[vartarget]], false);
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
	void ConstructBuffer(StringHandle, VM::ExecutionContext& context)
	{
		Integer32 size = context.State.Stack.PopValue<Integer32>();
		StringHandle identifierhandle = context.State.Stack.PopValue<StringHandle>();

		BufferHandle bufferhandle = context.OwnerVM.AllocateBuffer((size + 1) * sizeof(wchar_t));
		context.Variables->Write(identifierhandle, bufferhandle);
		context.TickBufferGarbageCollector();
	}


	void ConstructNothing(StringHandle, VM::ExecutionContext& context)
	{
		context.State.Stack.PopValue<Integer32>();
		context.State.Stack.PopValue<StringHandle>();
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
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"nothing"), ConstructNothing));
}

//
// Bind the library to a function metadata table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"value", Metadata::EpochType_Integer, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"integer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"value", Metadata::EpochType_Integer16, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"integer16"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"value", Metadata::EpochType_String, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"string"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"value", Metadata::EpochType_Boolean, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"boolean"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"value", Metadata::EpochType_Real, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"real"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"size", Metadata::EpochType_Integer, false);
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"buffer"), signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddPatternMatchedParameterIdentifier(stringpool.Pool(L"nothing"));
		AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"nothing"), signature));
	}
}

//
// Bind the library to the compiler's internal semantic action table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionCompileHelperTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"integer"), &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"integer16"), &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"string"), &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"boolean"), &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"real"), &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"buffer"), &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"nothing"), &CompileConstructorHelper));
}


void TypeConstructors::RegisterJITTable(JIT::JITTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"integer"), &ConstructIntegerJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(stringpool.Pool(L"real"), &ConstructRealJIT));
}
