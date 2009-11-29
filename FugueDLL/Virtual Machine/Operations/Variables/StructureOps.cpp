//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with structures
//

#include "pch.h"

#include "Virtual Machine/Operations/Variables/StructureOps.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/VMExceptions.h"
#include "Virtual Machine/SelfAware.inl"


using namespace VM;
using namespace VM::Operations;


//
// Construct and initialize the structure read operation
//
ReadStructure::ReadStructure(const std::wstring& varname, const std::wstring& membername)
	: VarName(varname),
	  MemberName(membername)
{
}

//
// Read a value from a structure
//
RValuePtr ReadStructure::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	return scope.GetVariableRef<StructureVariable>(VarName).ReadMember(MemberName);
}

void ReadStructure::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	// Nothing to do.
}

//
// Retrieve the type of the member to be read
//
EpochVariableTypeID ReadStructure::GetType(const ScopeDescription& scope) const
{
	return scope.GetStructureType(scope.GetVariableStructureTypeID(VarName)).GetMemberType(MemberName);
}

//
// Construct and initialize the structure read operation
//
ReadStructureIndirect::ReadStructureIndirect(const std::wstring& membername, Operation* priorop)
	: MemberName(membername),
	  PriorOp(priorop)			// This op is NOT owned by us, so don't delete it!
{
}

//
// Read a value from a structure on the stack
//
RValuePtr ReadStructureIndirect::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	StructureVariable thestruct(stack.GetCurrentTopOfStack());
	RValuePtr ret(thestruct.ReadMember(MemberName));
	stack.Pop(thestruct.GetStorageSize());
	return ret;
}

void ReadStructureIndirect::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	StructureVariable thestruct(stack.GetCurrentTopOfStack());
	stack.Pop(thestruct.GetStorageSize());
}

//
// Retrieve the type of the member to be read
//
EpochVariableTypeID ReadStructureIndirect::GetType(const ScopeDescription& scope) const
{
	IDType structid = WalkInstructionsForReadStruct(scope, PriorOp);
	return StructureTrackerClass::GetOwnerOfStructureType(structid)->GetStructureType(structid).GetMemberType(MemberName);
}

//
// Walk up the instruction list until we find the base structure
// variable that is being read from; then walk back down the list
// and determine the final type of the read expressions.
//
IDType ReadStructureIndirect::WalkInstructionsForReadStruct(const ScopeDescription& scope, Operation* op) const
{
	PushOperation* pushop = dynamic_cast<PushOperation*>(op);
	if(!pushop)
		throw InternalFailureException("READSTRUCTINDIRECT instruction without prior matching READSTRUCT");

	ReadStructure* readop = dynamic_cast<ReadStructure*>(pushop->GetNestedOperation());
	if(readop)
	{
		IDType structid = scope.GetVariableStructureTypeID(readop->VarName);
		return StructureTrackerClass::GetOwnerOfStructureType(structid)->GetStructureType(structid).GetMemberTypeHint(readop->MemberName);
	}

	ReadStructureIndirect* readindirectop = dynamic_cast<ReadStructureIndirect*>(pushop->GetNestedOperation());
	if(!readindirectop)
		throw InternalFailureException("READSTRUCTINDIRECT instruction has an invalid previous instruction");

	IDType structid = WalkInstructionsForReadStruct(scope, readindirectop->PriorOp);
	return StructureTrackerClass::GetOwnerOfStructureType(structid)->GetStructureType(structid).GetMemberTypeHint(readindirectop->MemberName);
}

//
// Construct and initialize the structure write operation
//
AssignStructure::AssignStructure(const std::wstring& varname, const std::wstring& membername)
	: VarName(varname),
	  MemberName(membername)
{
}

//
// Write a value to a structure
//
RValuePtr AssignStructure::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	StructureVariable& structure = scope.GetVariableRef<StructureVariable>(VarName);
	switch(GetType(scope.GetOriginalDescription()))
	{
	case EpochVariableType_Integer:
		{
			IntegerVariable var(stack.GetCurrentTopOfStack());
			structure.WriteMember(MemberName, var.GetAsRValue(), false);
			stack.Pop(IntegerVariable::GetStorageSize());
		}
		break;
	case EpochVariableType_Integer16:
		{
			Integer16Variable var(stack.GetCurrentTopOfStack());
			structure.WriteMember(MemberName, var.GetAsRValue(), false);
			stack.Pop(Integer16Variable::GetStorageSize());
		}
		break;
	case EpochVariableType_Real:
		{
			RealVariable var(stack.GetCurrentTopOfStack());
			structure.WriteMember(MemberName, var.GetAsRValue(), false);
			stack.Pop(RealVariable::GetStorageSize());
		}
		break;
	case EpochVariableType_Boolean:
		{
			BooleanVariable var(stack.GetCurrentTopOfStack());
			structure.WriteMember(MemberName, var.GetAsRValue(), false);
			stack.Pop(BooleanVariable::GetStorageSize());
		}
		break;
	case EpochVariableType_String:
		{
			StringVariable var(stack.GetCurrentTopOfStack());
			structure.WriteMember(MemberName, var.GetAsRValue(), false);
			stack.Pop(StringVariable::GetStorageSize());
		}
		break;
	case EpochVariableType_Structure:
		{
			StructureVariable var(stack.GetCurrentTopOfStack());
			const StructureType& structuretype = StructureTrackerClass::GetOwnerOfStructureType(structure.GetValue())->GetStructureType(structure.GetValue());
			if(var.GetValue() != structuretype.GetMemberTypeHint(MemberName))
				throw InternalFailureException("Incorrect structure type");

			const StructureType& substructuretype = StructureTrackerClass::GetOwnerOfStructureType(var.GetValue())->GetStructureType(var.GetValue());
			std::auto_ptr<StructureRValue> rvptr(new StructureRValue(substructuretype, var.GetValue()));
			const std::vector<std::wstring>& members = substructuretype.GetMemberOrder();
			for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				rvptr->AddMember(*iter, RValuePtr(var.ReadMember(*iter)->Clone()));

			structure.WriteMember(MemberName, RValuePtr(rvptr), false);
			stack.Pop(substructuretype.GetTotalSize());
		}
		break;
	case EpochVariableType_Function:
		{
			FunctionBinding var(stack.GetCurrentTopOfStack());
			structure.WriteMember(MemberName, var.GetAsRValue(), false);
			stack.Pop(FunctionBinding::GetStorageSize());
		}
		break;
	default:
		throw NotImplementedException("Cannot assign structure member value");
	}

	return structure.ReadMember(MemberName);
}

void AssignStructure::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

//
// Retrieve the type of the member to be written
//
EpochVariableTypeID AssignStructure::GetType(const ScopeDescription& scope) const
{
	return scope.GetStructureType(scope.GetVariableStructureTypeID(VarName)).GetMemberType(MemberName);
}

//
// Construct and initialize the structure write operation
//
AssignStructureIndirect::AssignStructureIndirect(const std::wstring& membername)
	: MemberName(membername)
{
}

//
// Write a value to a structure
//
RValuePtr AssignStructureIndirect::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	AddressVariable var(stack.GetCurrentTopOfStack());
	void* address = var.GetValue();
	stack.Pop(AddressVariable::GetStorageSize());

	StructureVariable structvar(address);
	const StructureType& structtype = scope.GetStructureType(structvar.GetValue());

	Byte* varaddress = reinterpret_cast<Byte*>(address) + structtype.GetMemberOffset(MemberName);

	switch(structtype.GetMemberType(MemberName))
	{
	case EpochVariableType_Integer:
		{
			IntegerVariable value(varaddress);
			IntegerVariable::BaseStorage newvalue = *reinterpret_cast<IntegerVariable::BaseStorage*>(stack.GetCurrentTopOfStack());
			value.SetValue(newvalue);
			stack.Pop(value.GetStorageSize());
			return value.GetAsRValue();
		}
	case EpochVariableType_Integer16:
		{
			Integer16Variable value(varaddress);
			Integer16Variable::BaseStorage newvalue = *reinterpret_cast<Integer16Variable::BaseStorage*>(stack.GetCurrentTopOfStack());
			value.SetValue(newvalue);
			stack.Pop(value.GetStorageSize());
			return value.GetAsRValue();
		}
	case EpochVariableType_Real:
		{
			RealVariable value(varaddress);
			RealVariable::BaseStorage newvalue = *reinterpret_cast<RealVariable::BaseStorage*>(stack.GetCurrentTopOfStack());
			value.SetValue(newvalue);
			stack.Pop(value.GetStorageSize());
			return value.GetAsRValue();
		}
	case EpochVariableType_String:
		{
			StringVariable value(varaddress);
			StringVariable::BaseStorage newvalue = *reinterpret_cast<StringVariable::BaseStorage*>(stack.GetCurrentTopOfStack());
			value.SetHandleValue(newvalue);
			stack.Pop(value.GetStorageSize());
			return value.GetAsRValue();
		}
	case EpochVariableType_Boolean:
		{
			BooleanVariable value(varaddress);
			BooleanVariable::BaseStorage newvalue = *reinterpret_cast<BooleanVariable::BaseStorage*>(stack.GetCurrentTopOfStack());
			value.SetValue(newvalue);
			stack.Pop(value.GetStorageSize());
			return value.GetAsRValue();
		}
	case EpochVariableType_Function:
		{
			FunctionBinding value(varaddress);
			FunctionBinding::BaseStorage newvalue = *reinterpret_cast<FunctionBinding::BaseStorage*>(stack.GetCurrentTopOfStack());

			if(!scope.GetFunctionSignature(structtype.GetMemberTypeHintString(MemberName)).DoesFunctionMatchSignature(newvalue, scope.GetOriginalDescription()))
				throw ExecutionException("Function does not meet the type requirements for this member");

			value.SetValue(newvalue);
			stack.Pop(value.GetStorageSize());
			return value.GetAsRValue();
		}
	case EpochVariableType_Address:
		{
			AddressVariable value(varaddress);
			AddressVariable::BaseStorage newvalue = *reinterpret_cast<AddressVariable::BaseStorage*>(stack.GetCurrentTopOfStack());
			value.SetValue(newvalue);
			stack.Pop(value.GetStorageSize());
			return value.GetAsRValue();
		}
	}

	throw NotImplementedException("Cannot assign nested structure value");
}

void AssignStructureIndirect::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}


//
// Retrieve the type of the member to be written
//
EpochVariableTypeID AssignStructureIndirect::GetType(const ScopeDescription& scope) const
{
	return EpochVariableType_Null;
}

//
// Construct and initialize the member reference operation
//
BindStructMemberReference::BindStructMemberReference(const std::wstring& membername)
	: VarName(NULL),
	  MemberName(membername),
	  Chained(true)
{
}

BindStructMemberReference::BindStructMemberReference(const std::wstring& varname, const std::wstring& membername)
	: VarName(&varname),
	  MemberName(membername),
	  Chained(false)
{
}

//
// Bind a reference to a given structure member
//
RValuePtr BindStructMemberReference::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	if(Chained)
	{
		AddressVariable addressvar(stack.GetCurrentTopOfStack());
		StructureVariable structvar(addressvar.GetValue());

		IDType structtype = structvar.GetValue();
		size_t offset = scope.GetStructureType(structtype).GetMemberOffset(MemberName);
		Byte* retaddr = reinterpret_cast<Byte*>(addressvar.GetValue()) + offset;

		stack.Pop(AddressVariable::GetStorageSize());
		return RValuePtr(new AddressRValue(retaddr));
	}
	else
	{
		Byte* address = reinterpret_cast<Byte*>(scope.GetVariableRef(*VarName).GetStorage()) + scope.GetStructureType(scope.GetVariableStructureTypeID(*VarName)).GetMemberOffset(MemberName);
		return RValuePtr(new AddressRValue(address));
	}
}

void BindStructMemberReference::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

//
// Retrieve the type of the member to be accessed
//
EpochVariableTypeID BindStructMemberReference::GetType(const ScopeDescription& scope) const
{
	return EpochVariableType_Address;
}

