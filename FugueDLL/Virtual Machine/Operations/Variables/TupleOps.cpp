//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with tuples
//

#include "pch.h"

#include "Virtual Machine/Operations/Variables/TupleOps.h"

#include "Virtual Machine/Core Entities/Variables/TupleVariable.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/SelfAware.inl"

#include "Virtual Machine/Types Management/Typecasts.h"

#include "Virtual Machine/VMExceptions.h"


using namespace VM;
using namespace VM::Operations;


//
// Construct and initialize the tuple read operation
//
ReadTuple::ReadTuple(const std::wstring& varname, const std::wstring& membername)
	: VarName(varname),
	  MemberName(membername)
{
}

//
// Read a value from a tuple
//
RValuePtr ReadTuple::ExecuteAndStoreRValue(ExecutionContext& context)
{
	return context.Scope.GetVariableRef<TupleVariable>(VarName).ReadMember(MemberName);
}

void ReadTuple::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do.
}

//
// Retrieve the type of the member to be read
//
EpochVariableTypeID ReadTuple::GetType(const ScopeDescription& scope) const
{
	return scope.GetTupleType(scope.GetVariableTupleTypeID(VarName)).GetMemberType(MemberName);
}

//
// Construct and initialize the tuple write operation
//
AssignTuple::AssignTuple(const std::wstring& varname, const std::wstring& membername)
	: VarName(varname),
	  MemberName(membername)
{
}

//
// Write a value to a tuple
//
RValuePtr AssignTuple::ExecuteAndStoreRValue(ExecutionContext& context)
{
	TupleVariable& tuple = context.Scope.GetVariableRef<TupleVariable>(VarName);
	switch(GetType(context.Scope.GetOriginalDescription()))
	{
	case EpochVariableType_Integer:
		{
			IntegerVariable var(context.Stack.GetCurrentTopOfStack());
			tuple.WriteMember(MemberName, var.GetAsRValue(), false);
			context.Stack.Pop(IntegerVariable::GetStorageSize());
		}
		break;
	case EpochVariableType_Real:
		{
			RealVariable var(context.Stack.GetCurrentTopOfStack());
			tuple.WriteMember(MemberName, var.GetAsRValue(), false);
			context.Stack.Pop(RealVariable::GetStorageSize());
		}
		break;
	case EpochVariableType_Boolean:
		{
			BooleanVariable var(context.Stack.GetCurrentTopOfStack());
			tuple.WriteMember(MemberName, var.GetAsRValue(), false);
			context.Stack.Pop(BooleanVariable::GetStorageSize());
		}
		break;
	case EpochVariableType_String:
		{
			StringVariable var(context.Stack.GetCurrentTopOfStack());
			tuple.WriteMember(MemberName, var.GetAsRValue(), false);
			context.Stack.Pop(StringVariable::GetStorageSize());
		}
		break;
	default:
		throw NotImplementedException("Cannot assign tuple member value");
	}

	return tuple.ReadMember(MemberName);
}

void AssignTuple::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

//
// Retrieve the type of the member to be written
//
EpochVariableTypeID AssignTuple::GetType(const ScopeDescription& scope) const
{
	return scope.GetTupleType(scope.GetVariableTupleTypeID(VarName)).GetMemberType(MemberName);
}
