//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// External function management routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Function.h"

#include "Marshalling/ExternalDLL.h"
#include "Marshalling/Libraries.h"


using namespace Parser;


//
// Track the name of the external DLL function currently being parsed.
//
void ParserState::RegisterExternalFunctionName(const std::wstring& functionname)
{
	FunctionName = functionname;
	ReadingFunctionSignature = true;
}

//
// Track the filename of the DLL where the currently parsed function resides.
//
void ParserState::RegisterExternalDLL(const std::wstring& dllname)
{
	ExternalDLLName = dllname;
	ParamCount = 0;
}

//
// Register that an external DLL function has a return type of integer.
//
void ParserState::RegisterExternalIntegerReturn()
{
	RegisterExternal(VM::EpochVariableType_Integer);
}

void ParserState::RegisterExternalInt16Return()
{
	RegisterExternal(VM::EpochVariableType_Integer16);
}

//
// Register that an external DLL function has a return type of real.
//
void ParserState::RegisterExternalRealReturn()
{
	RegisterExternal(VM::EpochVariableType_Real);
}

//
// Register that an external DLL function has a return type of string.
//
void ParserState::RegisterExternalStringReturn()
{
	RegisterExternal(VM::EpochVariableType_String);
}

//
// Register that an external DLL function has a return type of boolean.
//
void ParserState::RegisterExternalBooleanReturn()
{
	RegisterExternal(VM::EpochVariableType_Boolean);
}

//
// Register that an external DLL function has a return type of byte buffer.
//
void ParserState::RegisterExternalBufferReturn()
{
	RegisterExternal(VM::EpochVariableType_Buffer);
}

//
// Register that an external DLL function does not return a value.
//
void ParserState::RegisterExternalNullReturn()
{
	RegisterExternal(VM::EpochVariableType_Null);
}

//
// Register that an external DLL function accepts a pointer to the
// given type as a parameter
//
void ParserState::RegisterExternalParamAddressType(const std::wstring& paramtype)
{
	VM::EpochVariableTypeID addresstype;
	IDType addresstypehint;
		
	{
		if(CurrentScope->HasStructureType(paramtype))
		{
			addresstype = VM::EpochVariableType_Structure;
			addresstypehint = CurrentScope->GetStructureTypeID(paramtype);
		}
		else
		{
			if(CurrentScope->HasTupleType(paramtype))
				ReportFatalError("Cannot pass tuples to external functions; use a structure instead");
			else
				ReportFatalError("Unrecognized or invalid type - cannot pass address of this type to external function");

			addresstype = VM::EpochVariableType_Error;
			addresstypehint = VM::StructureTrackerClass::InvalidID;
		}
	}

	VariableTypeStack.push(addresstype);
	VariableHintStack.push(addresstypehint);
	ParamsByRef.push(false);
}

//
// Register that an external DLL function accepts a pointer
// with the given name as a parameter
//
void ParserState::RegisterExternalParamAddressName(const std::wstring& paramname)
{
	VariableNameStack.push(paramname);
	++ParamCount;	
}


//
// Register an external function call and bind it
// to the current scope.
//
void ParserState::RegisterExternal(VM::EpochVariableTypeID returntype)
{
	VM::EpochVariableTypeID returntypehint = VM::EpochVariableType_Error;
	std::auto_ptr<VM::ScopeDescription> params(new VM::ScopeDescription);

	params->ParentScope = CurrentScope;

	for( ; ParamCount > 0; --ParamCount)
	{
		switch(VariableTypeStack.top())
		{
		case VM::EpochVariableType_Structure:
			if(ParamsByRef.top())
			{
				params->AddReference(VM::EpochVariableType_Structure, VariableNameStack.top());
				params->SetVariableStructureTypeID(VariableNameStack.top(), VariableHintStack.top());
			}
			else
				params->AddStructureVariable(CurrentScope->GetStructureTypeID(VariableHintStack.top()), VariableNameStack.top());
			VariableHintStack.pop();
			break;

		case VM::EpochVariableType_Function:
			if(ParamsByRef.top())
			{
				ReportFatalError("Cannot pass functions by reference");
				HigherOrderFunctionHintStack.pop();
				break;
			}
			params->AddFunctionSignature(VariableNameStack.top(), HigherOrderFunctionHintStack.top(), true);
			HigherOrderFunctionHintStack.pop();
			break;

		case VM::EpochVariableType_Tuple:
			ReportFatalError("Cannot pass tuples to external functions; use a structure instead");
			break;

		case VM::EpochVariableType_Null:
			ReportFatalError("Cannot pass null/untyped values to external functions!");
			break;

		case VM::EpochVariableType_Error:
			// If this type is used, something already barfed in the parser,
			// so there's no point in showing another spurious error message
			break;

		default:
			if(ParamsByRef.top())
				params->AddReference(VariableTypeStack.top(), VariableNameStack.top());
			else
				params->AddVariable(VariableNameStack.top(), VariableTypeStack.top());
			break;
		}
		
		VariableTypeStack.pop();
		VariableNameStack.pop();
		ParamsByRef.pop();
	}

	std::auto_ptr<VM::FunctionBase> dllcall(new Marshalling::CallDLL(ExternalDLLName, FunctionName, params.release(), returntype, returntypehint));

	try
	{
		CurrentScope->AddFunction(FunctionName, dllcall);
	}
	catch(Exception& e)
	{
		ReportFatalError(e.what());
	}

	ReadingFunctionSignature = false;
}


//
// Load and link to an external library, making the library's
// functions, variables, and types available for use
//
void ParserState::RegisterLibrary(const std::wstring& filename)
{
	Marshalling::BindToLibrary(filename, *ParsedProgram);
}

