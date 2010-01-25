//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Function management routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Function.h"

#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"

#include "Virtual Machine/VMExceptions.h"


using namespace Parser;


//
// Register that a function definition follows.
// In the final parse phase, this information is used to
// bind the function body's statement block to the actual
// function itself.
//
void ParserState::RegisterUpcomingFunction(const std::wstring& functionname)
{
	PushIdentifier(functionname);
	FunctionName = functionname;
	ExpectedBlockTypes.push(BlockEntry::BLOCKENTRYTYPE_FUNCTION_NOCREATE);
}

//
// Register that a function definition follows.
// In the preparse phase, we use this information to
// create the actual function definition within the
// appropriate scope.
//
void ParserState::RegisterUpcomingFunctionPP(const std::wstring& functionname)
{
	PushIdentifier(functionname);
	ExpectedBlockTypes.push(BlockEntry::BLOCKENTRYTYPE_FUNCTION);
	ParamCount = 0;
	ReadingFunctionSignature = true;

	if(FunctionIsInfix)
		RegisterInfixFunction(functionname);

	FunctionIsInfix = false;
}


//
// Register a named parameter belonging to the current function.
//
void ParserState::RegisterParam(const std::wstring& paramname)
{
	VariableNameStack.push(paramname);
	++ParamCount;
}

//
// Prepare to receive an integer parameter name from the parser.
//
void ParserState::RegisterUpcomingIntegerParam()
{
	VariableTypeStack.push(VM::EpochVariableType_Integer);
	ParamsByRef.push(false);
}

void ParserState::RegisterUpcomingInt16Param()
{
	VariableTypeStack.push(VM::EpochVariableType_Integer16);
	ParamsByRef.push(false);
}


//
// Prepare to receive a real parameter name from the parser.
//
void ParserState::RegisterUpcomingRealParam()
{
	VariableTypeStack.push(VM::EpochVariableType_Real);
	ParamsByRef.push(false);
}

//
// Prepare to receive a string parameter name from the parser.
//
void ParserState::RegisterUpcomingStringParam()
{
	VariableTypeStack.push(VM::EpochVariableType_String);
	ParamsByRef.push(false);
}

//
// Prepare to receive a boolean parameter name from the parser.
//
void ParserState::RegisterUpcomingBooleanParam()
{
	VariableTypeStack.push(VM::EpochVariableType_Boolean);
	ParamsByRef.push(false);
}

//
// Prepare to receive a buffer parameter name from the parser.
//
void ParserState::RegisterUpcomingBufferParam()
{
	VariableTypeStack.push(VM::EpochVariableType_Buffer);
	ParamsByRef.push(false);
}

//
// Prepare to recieve a parameter of a user-defined type from the parser.
//
void ParserState::RegisterUpcomingUnknownParam(const std::wstring& nameoftype)
{
	if(CurrentScope->HasTupleType(nameoftype))
	{
		VariableTypeStack.push(VM::EpochVariableType_Tuple);
		VariableHintStack.push(CurrentScope->GetTupleTypeID(nameoftype));
	}
	else if(CurrentScope->HasStructureType(nameoftype))
	{
		VariableTypeStack.push(VM::EpochVariableType_Structure);
		VariableHintStack.push(CurrentScope->GetStructureTypeID(nameoftype));
	}
	else if(CurrentScope->IsFunctionSignature(nameoftype))
	{
		VariableTypeStack.push(VM::EpochVariableType_Function);
		HigherOrderFunctionHintStack.push(CurrentScope->GetFunctionSignature(nameoftype));
	}
	else
	{
		ReportFatalError("This type is not recognized or is not suitable for passing function parameters");
		VariableTypeStack.push(VM::EpochVariableType_Error);
	}

	ParamsByRef.push(false);
}

//
// Register that a parameter should be passed by reference
//
void ParserState::RegisterParamIsReference()
{
	ParamsByRef.top() = true;
}



//
// Prepare to receive a list of function return variables from the parser.
//
void ParserState::RegisterBeginningOfFunctionReturns()
{
	FunctionReturns = new VM::ScopeDescription;
	FunctionReturns->ParentScope = CurrentScope;
}

//
// Register that the current function has an integer-type return variable.
//
void ParserState::RegisterIntegerReturn(const std::wstring& retname)
{
	RegisterFunctionReturn(VM::EpochVariableType_Integer, retname);
}

void ParserState::RegisterInt16Return(const std::wstring& retname)
{
	RegisterFunctionReturn(VM::EpochVariableType_Integer16, retname);
}


//
// Register that the current function has a real-type return variable.
//
void ParserState::RegisterRealReturn(const std::wstring& retname)
{
	RegisterFunctionReturn(VM::EpochVariableType_Real, retname);
}

//
// Register that the current function has a string-type return variable.
//
void ParserState::RegisterStringReturn(const std::wstring& retname)
{
	RegisterFunctionReturn(VM::EpochVariableType_String, retname);
}

//
// Register that the current function has a boolean-type return variable.
//
void ParserState::RegisterBooleanReturn(const std::wstring& retname)
{
	RegisterFunctionReturn(VM::EpochVariableType_Boolean, retname);
}

//
// Register that the current function has an unknown return variable type.
// The type's name is passed first to this function, followed by the actual
// variable's name which is passed to RegisterUnknownReturnName.
//
void ParserState::RegisterUnknownReturn(const std::wstring& rettype)
{
	if(!CurrentScope->HasStructureType(rettype))
	{
		ReportFatalError("This type is not recognized or is not suitable for a function return value");
		return;
	}

	UnknownReturnTypes.push(rettype);
	UnknownReturnTypeHints.push(CurrentScope->GetStructureTypeID(rettype));

	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw ParserFailureException("Lost track of the name of the function being parsed");

	if(!FunctionReturnInitializationBlocks[TheStack.back().StringValue])
		FunctionReturnInitializationBlocks[TheStack.back().StringValue] = new VM::Block;
}

//
// Register the name of a structure-typed function return variable
//
void ParserState::RegisterUnknownReturnName(const std::wstring& retname)
{
	RegisterFunctionReturn(VM::EpochVariableType_Structure, retname);
}

//
// Register that we have finished parsing the constructor of a structure-typed return value
//
void ParserState::ExitUnknownReturnConstructor()
{
	std::wstring rettype = UnknownReturnTypes.top();
	UnknownReturnTypes.pop();

	if(FunctionReturnTypes.top() != VM::EpochVariableType_Structure)
		throw ParserFailureException("Expected a structure here");

	const std::wstring& varname = ParsedProgram->PoolStaticString(FunctionReturnNames.top());
	FunctionReturns->AddStructureVariable(rettype, varname);

	FunctionReturnTypes.pop();
	FunctionReturnNames.pop();
}

//
// Register that the instructions used to initialize a function's return values are completed
//
void ParserState::FinishReturnConstructor()
{
	IDType hint = UnknownReturnTypeHints.top();
	const VM::StructureType& t = VM::StructureTrackerClass::GetOwnerOfStructureType(hint)->GetStructureType(hint);
	const std::vector<std::wstring>& memberorder = t.GetMemberOrder();

	if(PassedParameterCount.top() - 1 != memberorder.size())
	{
		ReportFatalError("Incorrect number of parameters");

		for(unsigned i = 0; i < PassedParameterCount.top() - 1; ++i)
			TheStack.pop_back();
	}
	else
	{
		for(unsigned i = 0; i < PassedParameterCount.top() - 1; ++i)
		{
			if(TheStack.back().DetermineEffectiveType(*CurrentScope) != t.GetMemberType(memberorder[PassedParameterCount.top() - i - 2]))
				ReportFatalError("Type mismatch");

			TheStack.pop_back();
		}
	}

	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw ParserFailureException("Expected a structure identifier here");

	const std::wstring& varname = ParsedProgram->PoolStaticString(TheStack.back().StringValue);

	ReverseOpsAsGroups(FunctionReturnInitializationBlocks[FunctionName], PassedParameterCount.top() - 1);

	TheStack.pop_back();
	PopParameterCount();

	FunctionReturnInitializationBlocks[FunctionName]->AddOperation(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(static_cast<Integer32>(hint))));
	FunctionReturnInitializationBlocks[FunctionName]->AddOperation(VM::OperationPtr(new VM::Operations::InitializeValue(varname)));

	UnknownReturnTypeHints.pop();
}

//
// Register that the current function has a null return type.
//
void ParserState::RegisterNullReturn()
{
	if(!FunctionReturnTypes.empty() || !FunctionReturnNames.empty())
		throw ParserFailureException("Function defined as returning null, but return value names/types are on the parse stack. This most likely means something is broken in the parser.");
}

//
// Register the default integer value of the current return variable.
//
void ParserState::RegisterReturnValue(Integer32 value)
{
	if(FunctionReturnTypes.top() != VM::EpochVariableType_Integer)
		throw ParserFailureException("Expected an integer here");

	const std::wstring& varname = ParsedProgram->PoolStaticString(FunctionReturnNames.top());
	FunctionReturns->AddVariable(varname, VM::EpochVariableType_Integer);

	ReturnValEntry valentry;
	valentry.Type = VM::EpochVariableType_Integer;
	valentry.IntegerValue = value;
	FunctionReturnValueTracker[TheStack.back().StringValue].insert(FunctionRetEntry(varname, valentry));

	FunctionReturnTypes.pop();
	FunctionReturnNames.pop();
}

//
// Register the default real value of the current return variable.
//
void ParserState::RegisterReturnValue(Real value)
{
	if(FunctionReturnTypes.top() != VM::EpochVariableType_Real)
		throw ParserFailureException("Expected a real here");

	const std::wstring& varname = ParsedProgram->PoolStaticString(FunctionReturnNames.top());
	FunctionReturns->AddVariable(varname, VM::EpochVariableType_Real);

	ReturnValEntry valentry;
	valentry.Type = VM::EpochVariableType_Real;
	valentry.RealValue = value;
	FunctionReturnValueTracker[TheStack.back().StringValue].insert(FunctionRetEntry(varname, valentry));

	FunctionReturnTypes.pop();
	FunctionReturnNames.pop();
}

//
// Register the default string value of the current return variable.
//
void ParserState::RegisterReturnValue(const std::wstring& value)
{
	if(FunctionReturnTypes.top() != VM::EpochVariableType_String)
		throw ParserFailureException("Expected a string here");

	const std::wstring& varname = ParsedProgram->PoolStaticString(FunctionReturnNames.top());
	FunctionReturns->AddVariable(varname, VM::EpochVariableType_String);

	ReturnValEntry valentry;
	valentry.Type = VM::EpochVariableType_String;
	valentry.StringValue = value;
	FunctionReturnValueTracker[TheStack.back().StringValue].insert(FunctionRetEntry(varname, valentry));

	FunctionReturnTypes.pop();
	FunctionReturnNames.pop();
}

//
// Register the default boolean value of the current return variable.
//
void ParserState::RegisterReturnValue(bool value)
{
	if(FunctionReturnTypes.top() != VM::EpochVariableType_Boolean)
		throw ParserFailureException("Expected a boolean here");

	const std::wstring& varname = ParsedProgram->PoolStaticString(FunctionReturnNames.top());
	FunctionReturns->AddVariable(varname, VM::EpochVariableType_Boolean);

	ReturnValEntry valentry;
	valentry.Type = VM::EpochVariableType_Boolean;
	valentry.BooleanValue = value;
	FunctionReturnValueTracker[TheStack.back().StringValue].insert(FunctionRetEntry(varname, valentry));

	FunctionReturnTypes.pop();
	FunctionReturnNames.pop();
}


//
// Register the name and type of (one of) a function's return variable(s)
//
void ParserState::RegisterFunctionReturn(VM::EpochVariableTypeID type, const std::wstring& name)
{
	FunctionReturnTypes.push(type);
	FunctionReturnNames.push(name);
	ReadingFunctionSignature = false;
}

//
// Add operations to a function's code block to automatically
// initialize all return variables to their given defaults.
//
void ParserState::MergeFunctionReturns(const FunctionRetMap& functionreturnvalues, VM::Block& block)
{
	for(std::map<std::wstring, ReturnValEntry>::const_iterator iter = functionreturnvalues.begin(); iter != functionreturnvalues.end(); ++iter)
	{
		switch(iter->second.Type)
		{
		case VM::EpochVariableType_Null:
			throw ParserFailureException("Cannot return null value from a function; return an empty tuple instead");

		case VM::EpochVariableType_Integer:
			block.AddOperation(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(iter->second.IntegerValue)));
			break;
		case VM::EpochVariableType_Integer16:
			block.AddOperation(VM::OperationPtr(new VM::Operations::PushInteger16Literal(iter->second.IntegerValue)));
			break;
		case VM::EpochVariableType_Real:
			block.AddOperation(VM::OperationPtr(new VM::Operations::PushRealLiteral(iter->second.RealValue)));
			break;
		case VM::EpochVariableType_Boolean:
			block.AddOperation(VM::OperationPtr(new VM::Operations::PushBooleanLiteral(iter->second.BooleanValue)));
			break;
		case VM::EpochVariableType_String:
			block.AddOperation(VM::OperationPtr(new VM::Operations::PushStringLiteral(iter->second.StringValue)));
			break;

		default:
			throw VM::NotImplementedException("Cannot return this type from a function");
		}

		block.AddOperation(VM::OperationPtr(new VM::Operations::InitializeValue(ParsedProgram->PoolStaticString(iter->first))));
	}
}


//
// Register that we are defining a function signature with the given name
//
void ParserState::RegisterFunctionSignatureName(const std::wstring& identifier)
{
	FunctionSigStackEntry entry;
	entry.Name = identifier;
	FunctionSignatureStack.push(entry);
}

//
// Register a parameter in the current function signature
//
void ParserState::RegisterFunctionSignatureParam(VM::EpochVariableTypeID type)
{
	FunctionSignatureStack.top().Signature.AddParam(type, 0, NULL);
}

//
// Register a parameter of an unknown type in the current function signature
//
void ParserState::RegisterFunctionSignatureParam(const std::wstring& nameoftype)
{
	if(CurrentScope->HasTupleType(nameoftype))
		FunctionSignatureStack.top().Signature.AddParam(VM::EpochVariableType_Tuple, CurrentScope->GetTupleTypeID(nameoftype), NULL);
	else if(CurrentScope->HasStructureType(nameoftype))
		FunctionSignatureStack.top().Signature.AddParam(VM::EpochVariableType_Structure, CurrentScope->GetStructureTypeID(nameoftype), NULL);
	else if(CurrentScope->IsFunctionSignature(nameoftype))
		FunctionSignatureStack.top().Signature.AddParam(VM::EpochVariableType_Function, 0, new VM::FunctionSignature(CurrentScope->GetFunctionSignature(nameoftype)));
	else
	{
		ReportFatalError("This type is not recognized or is not suitable for passing function parameters");
		FunctionSignatureStack.top().Signature.AddParam(VM::EpochVariableType_Error, 0, NULL);
	}
}

//
// Register that a parameter in a function signature should be passed by reference
//
void ParserState::RegisterFunctionSignatureParamIsReference()
{
	try
	{
		FunctionSignatureStack.top().Signature.SetLastParamToReference();
	}
	catch(Exception& e)
	{
		ReportFatalError(e.what());
	}
}

//
// Register a return value in the current function signature
//
void ParserState::RegisterFunctionSignatureReturn(VM::EpochVariableTypeID type)
{
	FunctionSignatureStack.top().Signature.AddReturn(type, 0);
}

//
// Register a return value of an unknown type in the current function signature
//
void ParserState::RegisterFunctionSignatureReturn(const std::wstring& nameoftype)
{
	if(CurrentScope->HasTupleType(nameoftype))
		FunctionSignatureStack.top().Signature.AddReturn(VM::EpochVariableType_Tuple, CurrentScope->GetTupleTypeID(nameoftype));
	else if(CurrentScope->HasStructureType(nameoftype))
		FunctionSignatureStack.top().Signature.AddReturn(VM::EpochVariableType_Structure, CurrentScope->GetStructureTypeID(nameoftype));
	else
		ReportFatalError("This type is not recognized or is not suitable for a function return value");
}

//
// Register that we have finished parsing a function signature, and
// can now finalize the signature and add it to the current scope.
//
void ParserState::RegisterFunctionSignatureEnd()
{
	if(FunctionSignatureStack.size() == 1)
	{
		if(ReadingFunctionSignature)
		{
			VariableNameStack.push(FunctionSignatureStack.top().Name);
			VariableTypeStack.push(VM::EpochVariableType_Function); 
			HigherOrderFunctionHintStack.push(FunctionSignatureStack.top().Signature);

			++ParamCount;
			ParamsByRef.push(false);
			FunctionSignatureStack.pop();
		}
		else
		{
			CurrentScope->AddFunctionSignature(FunctionSignatureStack.top().Name, FunctionSignatureStack.top().Signature, true);
			FunctionSignatureStack.pop();
		}
	}
	else
	{
		std::auto_ptr<VM::FunctionSignature> signature(new VM::FunctionSignature(FunctionSignatureStack.top().Signature));
		FunctionSignatureStack.pop();
		FunctionSignatureStack.top().Signature.AddParam(VM::EpochVariableType_Function, 0, signature.release());
	}
}
