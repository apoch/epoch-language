//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - types management operations
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Types Management/RuntimeCasts.h"
#include "Virtual Machine/Types Management/TypeInfo.h"

#include "Virtual Machine/VMExceptions.h"

#include "Virtual Machine/SelfAware.inl"


using namespace Parser;


//
// Create an operation to convert data in one format into another
//
VM::OperationPtr ParserState::CreateOperation_Cast()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("cast() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry p1 = TheStack.back();
	TheStack.pop_back();

	StackEntry p2 = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID vartype = VM::EpochVariableType_Error;


	try
	{
		vartype = p1.DetermineEffectiveType(*CurrentScope);
	}
	catch(VM::MissingVariableException& e)
	{
		ReportFatalError(e.what());
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(p2.StringValue == Keywords::Integer)
	{
		switch(vartype)
		{
		case VM::EpochVariableType_String:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::IntegerT>);
		case VM::EpochVariableType_Real:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::RealT, TypeInfo::IntegerT>);
		case VM::EpochVariableType_Integer:
			ReportFatalError("Value is already an integer; no need to cast");
			return VM::OperationPtr(new VM::Operations::NoOp);
		case VM::EpochVariableType_Integer16:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::Integer16T, TypeInfo::IntegerT>);
		case VM::EpochVariableType_Boolean:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::IntegerT>);
		case VM::EpochVariableType_Tuple:
		case VM::EpochVariableType_Structure:
		case VM::EpochVariableType_Function:
		case VM::EpochVariableType_Address:
		case VM::EpochVariableType_Array:
		case VM::EpochVariableType_TaskHandle:
			ReportFatalError("This data type cannot be converted into an integer");
			return VM::OperationPtr(new VM::Operations::NoOp);
		default:
			throw VM::NotImplementedException("Conversion to integer is not implemented");
		}
	}
	else if(p2.StringValue == Keywords::Integer16)
	{
		switch(vartype)
		{
		case VM::EpochVariableType_String:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::Integer16T>);
		case VM::EpochVariableType_Real:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::RealT, TypeInfo::Integer16T>);
		case VM::EpochVariableType_Integer:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::IntegerT, TypeInfo::Integer16T>);
		case VM::EpochVariableType_Integer16:
			ReportFatalError("Value is already a 16-bit integer; no need to cast");
			return VM::OperationPtr(new VM::Operations::NoOp);
		case VM::EpochVariableType_Boolean:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::Integer16T>);
		case VM::EpochVariableType_Tuple:
		case VM::EpochVariableType_Structure:
		case VM::EpochVariableType_Function:
		case VM::EpochVariableType_Address:
		case VM::EpochVariableType_Array:
		case VM::EpochVariableType_TaskHandle:
			ReportFatalError("This data type cannot be converted into an integer16");
			return VM::OperationPtr(new VM::Operations::NoOp);
		default:
			throw VM::NotImplementedException("Conversion to integer16 is not implemented");
		}
	}
	else if(p2.StringValue == Keywords::Real)
	{
		switch(vartype)
		{
		case VM::EpochVariableType_String:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::RealT>);
		case VM::EpochVariableType_Real:
			ReportFatalError("Value is already a real; no need to cast");
			return VM::OperationPtr(new VM::Operations::NoOp);
		case VM::EpochVariableType_Integer:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::IntegerT, TypeInfo::RealT>);
		case VM::EpochVariableType_Integer16:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::Integer16T, TypeInfo::RealT>);
		case VM::EpochVariableType_Boolean:
			return VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::RealT>);
		case VM::EpochVariableType_Tuple:
		case VM::EpochVariableType_Structure:
		case VM::EpochVariableType_Function:
		case VM::EpochVariableType_Address:
		case VM::EpochVariableType_Array:
		case VM::EpochVariableType_TaskHandle:
			ReportFatalError("This data type cannot be converted into a real");
			return VM::OperationPtr(new VM::Operations::NoOp);
		default:
			throw VM::NotImplementedException("Conversion to real is not implemented");
		}
	}
	else if(p2.StringValue == Keywords::Boolean)
	{
		ReportFatalError("Casting to boolean is not permitted; use a comparison operator instead");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	else if(p2.StringValue == Keywords::String)
	{
		switch(vartype)
		{
		case VM::EpochVariableType_String:
			ReportFatalError("Value is already a string; no need to cast");
			return VM::OperationPtr(new VM::Operations::NoOp);
		case VM::EpochVariableType_Real:
			return VM::OperationPtr(new VM::Operations::TypeCastToString<TypeInfo::RealT>);
		case VM::EpochVariableType_Integer:
			return VM::OperationPtr(new VM::Operations::TypeCastToString<TypeInfo::IntegerT>);
		case VM::EpochVariableType_Integer16:
			return VM::OperationPtr(new VM::Operations::TypeCastToString<TypeInfo::Integer16T>);
		case VM::EpochVariableType_Boolean:
			return VM::OperationPtr(new VM::Operations::TypeCastBooleanToString());
		case VM::EpochVariableType_Buffer:
			return VM::OperationPtr(new VM::Operations::TypeCastBufferToString());
		case VM::EpochVariableType_Tuple:
		case VM::EpochVariableType_Structure:
		case VM::EpochVariableType_Function:
		case VM::EpochVariableType_Address:
		case VM::EpochVariableType_Array:
		case VM::EpochVariableType_TaskHandle:
			ReportFatalError("This data type cannot be converted into a string");
			return VM::OperationPtr(new VM::Operations::NoOp);
		default:
			throw VM::NotImplementedException("Conversion to string is not implemented");
		}
	}

	throw VM::NotImplementedException("Conversion between these types is not available");
}