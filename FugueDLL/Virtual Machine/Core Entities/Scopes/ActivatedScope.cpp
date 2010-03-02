//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class for an active lexical scope
//


#include "pch.h"

#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"

#include "Virtual Machine/Core Entities/Concurrency/Future.h"

#include "Virtual Machine/Core Entities/Types/Tuple.h"
#include "Virtual Machine/Core Entities/Types/Structure.h"

#include "Virtual Machine/Core Entities/Variables/TupleVariable.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"
#include "Virtual Machine/Core Entities/Variables/ArrayVariable.h"

#include "Virtual Machine/Types Management/TypeInfo.h"

#include "Utility/Strings.h"

#include "Utility/Memory/Heap.h"


using namespace VM;


//-------------------------------------------------------------------------------
// Construction
//-------------------------------------------------------------------------------


//
// Construct and initialize an activated scope, based on a scope description template
//
ActivatedScope::ActivatedScope(ScopeDescription& scope)
	: OriginalScope(scope),
	  ParentScope(NULL),
	  TaskOrigin(0),
	  LastMessageOrigin(0),

	  Variables(scope.Variables),
	  References(scope.References),
	  Futures(scope.Futures)
{ }

//
// Construct and initialize an activated scope with a given parent scope, based on a scope description template
//
ActivatedScope::ActivatedScope(ScopeDescription& scope, ActivatedScope* parent)
	: OriginalScope(scope),
	  ParentScope(parent),
	  TaskOrigin(0),
	  LastMessageOrigin(0),

	  Variables(scope.Variables),
	  References(scope.References),
	  Futures(scope.Futures)
{ }

//
// Destruct and clean up an activated scope
//
ActivatedScope::~ActivatedScope()
{
	// Poison data members just in case someone tries to use this scope once it has been deleted
	ParentScope = reinterpret_cast<ActivatedScope*>(static_cast<UINT_PTR>(0xbaadf00d));
}


//-------------------------------------------------------------------------------
// Stack interaction
//-------------------------------------------------------------------------------

//
// Enter a lexical scope, reserving space for each variable on the stack.
//
void ActivatedScope::Enter(StackSpace& stack)
{
	StackUsage.push(0);

	for(std::vector<std::wstring>::const_iterator iter = OriginalScope.MemberOrder.begin(); iter != OriginalScope.MemberOrder.end(); ++iter)
	{
		VariableMap::iterator variter = Variables.find(*iter);
		if(variter == Variables.end())
			continue;

		Variable& var = variter->second;
		switch(var.GetType())
		{
		case EpochVariableType_Null:
			StackUsage.top() += var.CastTo<NullVariable>().BindToStack(stack);
			break;
		case EpochVariableType_Integer:
			StackUsage.top() += var.CastTo<IntegerVariable>().BindToStack(stack);
			break;
		case EpochVariableType_Integer16:
			StackUsage.top() += var.CastTo<Integer16Variable>().BindToStack(stack);
			break;
		case EpochVariableType_Real:
			StackUsage.top() += var.CastTo<RealVariable>().BindToStack(stack);
			break;
		case EpochVariableType_Boolean:
			StackUsage.top() += var.CastTo<BooleanVariable>().BindToStack(stack);
			break;
		case EpochVariableType_String:
			StackUsage.top() += var.CastTo<StringVariable>().BindToStack(stack);
			break;
		case EpochVariableType_Tuple:
			{
				ScopeDescription::TupleTypeIDMap::const_iterator ttiter = OriginalScope.TupleTypeHints.find(*iter);
				if(ttiter == OriginalScope.TupleTypeHints.end())
					throw Exception("Invalid type hint");
				StackUsage.top() += var.CastTo<TupleVariable>().BindToStack(stack, GetTupleType(ttiter->second));
			}
			break;
		case EpochVariableType_Structure:
			{
				ScopeDescription::StructureTypeIDMap::const_iterator stiter = OriginalScope.StructureTypeHints.find(*iter);
				if(stiter == OriginalScope.StructureTypeHints.end())
					throw Exception("Invalid type hint");
				StackUsage.top() += var.CastTo<StructureVariable>().BindToStack(stack, GetStructureType(stiter->second));
			}
			break;
		case EpochVariableType_Buffer:
			StackUsage.top() += var.CastTo<BufferVariable>().BindToStack(stack);
			break;
		case EpochVariableType_Array:
			StackUsage.top() += var.CastTo<ArrayVariable>().BindToStack(stack);
			break;
		default:
			throw NotImplementedException("Cannot reserve stack space for this variable type");
		}
	}
}

//
// Enter a lexical scope, storing information in a special heap container instead of on the stack
//
void ActivatedScope::Enter(HeapStorage& heapstorage)
{
	size_t requiredstorage = 0;

	for(std::vector<std::wstring>::const_iterator iter = OriginalScope.MemberOrder.begin(); iter != OriginalScope.MemberOrder.end(); ++iter)
	{
		VariableMap::iterator variter = Variables.find(*iter);
		if(variter == Variables.end())
			continue;

		Variable& var = variter->second;
		switch(var.GetType())
		{
		case EpochVariableType_Tuple:
			{
				ScopeDescription::TupleTypeIDMap::const_iterator ttiter = OriginalScope.TupleTypeHints.find(*iter);
				if(ttiter == OriginalScope.TupleTypeHints.end())
					throw Exception("Invalid type hint");
				requiredstorage += GetTupleType(ttiter->second).GetTotalSize();
			}
			break;
		case EpochVariableType_Structure:
			{
				ScopeDescription::StructureTypeIDMap::const_iterator stiter = OriginalScope.StructureTypeHints.find(*iter);
				if(stiter == OriginalScope.StructureTypeHints.end())
					throw Exception("Invalid type hint");
				requiredstorage += GetStructureType(stiter->second).GetTotalSize();
			}
			break;
		default:
			requiredstorage += TypeInfo::GetStorageSize(var.GetType());
			break;
		}
	}

	heapstorage.Allocate(requiredstorage);
	Byte* storage = reinterpret_cast<Byte*>(heapstorage.GetStartOfStorage());
	for(std::vector<std::wstring>::const_iterator iter = OriginalScope.MemberOrder.begin(); iter != OriginalScope.MemberOrder.end(); ++iter)
	{
		VariableMap::iterator variter = Variables.find(*iter);
		if(variter == Variables.end())
			continue;

		Variable& var = variter->second;
		var.BindToStorage(storage);
		switch(var.GetType())
		{
			{
				ScopeDescription::TupleTypeIDMap::const_iterator ttiter = OriginalScope.TupleTypeHints.find(*iter);
				if(ttiter == OriginalScope.TupleTypeHints.end())
					throw Exception("Invalid type hint");
				storage += GetTupleType(ttiter->second).GetTotalSize();
			}
			break;
		case EpochVariableType_Structure:
			{
				ScopeDescription::StructureTypeIDMap::const_iterator stiter = OriginalScope.StructureTypeHints.find(*iter);
				if(stiter == OriginalScope.StructureTypeHints.end())
					throw Exception("Invalid type hint");
				storage += GetStructureType(stiter->second).GetTotalSize();
			}
			break;
		default:
			storage += TypeInfo::GetStorageSize(var.GetType());
			break;
		}
	}
}

//
// Exit a lexical scope, popping off all the allocated space for variables.
//
void ActivatedScope::Exit(StackSpace& stack)
{
	if(StackUsage.empty())
		throw InternalFailureException("Exited scope too many times!");

	stack.Pop(StackUsage.top());
	StackUsage.pop();
}

//
// Exit a lexical scope that was bound to the host hardware rather than the VM stack.
//
void ActivatedScope::ExitFromMachineStack()
{
	StackUsage.pop();
}

//
// Bind the scope's variables to existing, allocated slots on the stack.
// This is primarily useful for functions, where the parameter scope can
// be bound to the stack to automatically populate the parameter variables
// with their passed values.
//
void ActivatedScope::BindToStack(StackSpace& stack)
{
	StackUsage.push(0);

	for(std::vector<std::wstring>::const_iterator iter = OriginalScope.MemberOrder.begin(); iter != OriginalScope.MemberOrder.end(); ++iter)
	{
		if(IsReference(*iter))
		{
			ReferenceBinding binding(stack.GetOffsetIntoStack(StackUsage.top()));
			StackUsage.top() += ReferenceBinding::GetStorageSize();

			References.find(*iter)->second.second = binding.GetValue();
		}
		else if(IsFunctionSignature(*iter))
		{
			FunctionBinding binding(stack.GetOffsetIntoStack(StackUsage.top()));
			StackUsage.top() += FunctionBinding::GetStorageSize();

			AddFunction(*iter, binding.GetValue());
		}
		else
		{
			VariableMap::iterator variter = Variables.find(*iter);
			if(variter == Variables.end())
				throw InternalFailureException("Incomplete variable bindings in scope; not sure how to proceed");

			Variable& var = variter->second;
			var.BindToStorage(stack.GetOffsetIntoStack(StackUsage.top()));
			switch(var.GetType())
			{
			case EpochVariableType_Tuple:
				StackUsage.top() += GetTupleType(var.CastTo<TupleVariable>().GetValue()).GetTotalSize();
				break;
			case EpochVariableType_Structure:
				StackUsage.top() += GetStructureType(var.CastTo<StructureVariable>().GetValue()).GetTotalSize();
				break;
			case EpochVariableType_Array:
				StackUsage.top() += var.CastTo<ArrayVariable>().GetStorageSize();
				break;
			default:
				StackUsage.top() += TypeInfo::GetStorageSize(var.GetType());
				break;
			}
		}
	}
}

//
// Bind the scope to a section of stack memory on the host hardware, rather
// than in the virtual machine. This is used primarily by the marshalling
// system which interacts with external code.
//
void ActivatedScope::BindToMachineStack(void* rawstack)
{
	StackUsage.push(0);

	UByte* topofstack = reinterpret_cast<UByte*>(rawstack);

	for(std::vector<std::wstring>::const_reverse_iterator iter = OriginalScope.MemberOrder.rbegin(); iter != OriginalScope.MemberOrder.rend(); ++iter)
	{
		if(IsReference(*iter))
		{
			ReferenceBinding binding(topofstack + StackUsage.top());
			StackUsage.top() += ReferenceBinding::GetStorageSize();

			References.find(*iter)->second.second = binding.GetValue();
		}
		else if(IsFunctionSignature(*iter))
		{
			FunctionBinding binding(topofstack + StackUsage.top());
			StackUsage.top() += FunctionBinding::GetStorageSize();

			AddFunction(*iter, binding.GetValue());
		}
		else
		{
			VariableMap::iterator variter = Variables.find(*iter);
			if(variter == Variables.end())
				throw InternalFailureException("Incomplete variable bindings in scope; not sure how to proceed");

			Variable& var = variter->second;
			var.BindToStorage(topofstack + StackUsage.top());
			switch(var.GetType())
			{
			case EpochVariableType_Tuple:
				StackUsage.top() += GetTupleType(var.CastTo<TupleVariable>().GetValue()).GetTotalSize();
				break;
			case EpochVariableType_Structure:
				StackUsage.top() += GetStructureType(var.CastTo<StructureVariable>().GetValue()).GetTotalSize();
				break;
			default:
				StackUsage.top() += TypeInfo::GetStorageSize(var.GetType());
				break;
			}
		}
	}
}


//
// Pop the top of the stack and save the value into the given variable.
// Primarily useful for assigning values on the stack into variables.
//
RValuePtr ActivatedScope::PopVariableOffStack(const std::wstring& name, StackSpace& stack, bool ignorestorage)
{
	return PopVariableOffStack(name, LookupVariable(name), stack, ignorestorage);
}

RValuePtr ActivatedScope::PopVariableOffStack(const std::wstring& name, Variable& var, StackSpace& stack, bool ignorestorage)
{
	switch(var.GetType())
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot assign value into null variable");
	case EpochVariableType_Integer:
		{
			IntegerVariable temp(stack.GetCurrentTopOfStack());
			RValuePtr ret(temp.GetAsRValue());
			var.CastTo<IntegerVariable>().SetValue(temp.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize());
			return ret;
		}
	case EpochVariableType_Integer16:
		{
			Integer16Variable temp(stack.GetCurrentTopOfStack());
			RValuePtr ret(temp.GetAsRValue());
			var.CastTo<Integer16Variable>().SetValue(temp.GetValue());
			stack.Pop(Integer16Variable::GetStorageSize());
			return ret;
		}
	case EpochVariableType_Real:
		{
			RealVariable temp(stack.GetCurrentTopOfStack());
			RValuePtr ret(temp.GetAsRValue());
			var.CastTo<RealVariable>().SetValue(temp.GetValue());
			stack.Pop(RealVariable::GetStorageSize());
			return ret;
		}
	case EpochVariableType_Boolean:
		{
			BooleanVariable temp(stack.GetCurrentTopOfStack());
			RValuePtr ret(temp.GetAsRValue());
			var.CastTo<BooleanVariable>().SetValue(temp.GetValue());
			stack.Pop(BooleanVariable::GetStorageSize());
			return ret;
		}
	case EpochVariableType_String:
		{
			StringVariable temp(stack.GetCurrentTopOfStack());
			RValuePtr ret(temp.GetAsRValue());
			var.CastTo<StringVariable>().SetHandleValue(temp.GetHandleValue());
			stack.Pop(StringVariable::GetStorageSize());
			return ret;
		}
	case EpochVariableType_Tuple:
		{
			IntegerVariable temp(stack.GetCurrentTopOfStack());
			TupleVariable::BaseStorage id = static_cast<TupleVariable::BaseStorage>(temp.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize());

			const TupleType& tupletype = TupleTrackerClass::GetOwnerOfTupleType(id)->GetTupleType(id);
			RValuePtr tupleretptr(new TupleRValue(tupletype, id));
			TupleRValue* tupleret = dynamic_cast<TupleRValue*>(tupleretptr.get());

			TupleVariable& tuplevar = var.CastTo<TupleVariable>();
			tuplevar.SetValue(id);

			const std::vector<std::wstring>& members = tupletype.GetMemberOrder();
			for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
			{
				switch(tupletype.GetMemberType(*iter))
				{
				case EpochVariableType_Integer:
					{
						IntegerVariable var(stack.GetCurrentTopOfStack());
						tupleret->AddMember(*iter, var.GetAsRValue());
						tuplevar.WriteMember(*iter, tupleret->GetValue(*iter), ignorestorage);
						stack.Pop(IntegerVariable::GetStorageSize());
					}
					break;
				case EpochVariableType_Integer16:
					{
						Integer16Variable var(stack.GetCurrentTopOfStack());
						tupleret->AddMember(*iter, var.GetAsRValue());
						tuplevar.WriteMember(*iter, tupleret->GetValue(*iter), ignorestorage);
						stack.Pop(Integer16Variable::GetStorageSize());
					}
					break;
				case EpochVariableType_Boolean:
					{
						BooleanVariable var(stack.GetCurrentTopOfStack());
						tupleret->AddMember(*iter, var.GetAsRValue());
						tuplevar.WriteMember(*iter, tupleret->GetValue(*iter), ignorestorage);
						stack.Pop(BooleanVariable::GetStorageSize());
					}
					break;
				case EpochVariableType_String:
					{
						StringVariable var(stack.GetCurrentTopOfStack());
						tupleret->AddMember(*iter, var.GetAsRValue());
						tuplevar.WriteMember(*iter, tupleret->GetValue(*iter), ignorestorage);
						stack.Pop(StringVariable::GetStorageSize());
					}
					break;
				case EpochVariableType_Real:
					{
						RealVariable var(stack.GetCurrentTopOfStack());
						tupleret->AddMember(*iter, var.GetAsRValue());
						tuplevar.WriteMember(*iter, tupleret->GetValue(*iter), ignorestorage);
						stack.Pop(RealVariable::GetStorageSize());
					}
					break;
				default:
					throw NotImplementedException("Cannot read tuple member of this type");
				}
			}

			return tupleretptr;
		}
	case EpochVariableType_Structure:
		{
			IntegerVariable temp(stack.GetCurrentTopOfStack());
			StructureVariable::BaseStorage id = static_cast<StructureVariable::BaseStorage>(temp.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize());

			const StructureType& structuretype = StructureTrackerClass::GetOwnerOfStructureType(id)->GetStructureType(id);
			RValuePtr structureretptr(new StructureRValue(structuretype, id));
			StructureRValue* structureret = dynamic_cast<StructureRValue*>(structureretptr.get());

			StructureVariable& structurevar = var.CastTo<StructureVariable>();
			structurevar.SetValue(id);

			const std::vector<std::wstring>& members = structuretype.GetMemberOrder();
			for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
			{
				switch(structuretype.GetMemberType(*iter))
				{
				case EpochVariableType_Integer:
					{
						IntegerVariable var(stack.GetCurrentTopOfStack());
						structureret->AddMember(*iter, var.GetAsRValue());
						structurevar.WriteMember(*iter, structureret->GetValue(*iter), ignorestorage);
						stack.Pop(IntegerVariable::GetStorageSize());
					}
					break;
				case EpochVariableType_Integer16:
					{
						Integer16Variable var(stack.GetCurrentTopOfStack());
						structureret->AddMember(*iter, var.GetAsRValue());
						structurevar.WriteMember(*iter, structureret->GetValue(*iter), ignorestorage);
						stack.Pop(Integer16Variable::GetStorageSize());
					}
					break;
				case EpochVariableType_Boolean:
					{
						BooleanVariable var(stack.GetCurrentTopOfStack());
						structureret->AddMember(*iter, var.GetAsRValue());
						structurevar.WriteMember(*iter, structureret->GetValue(*iter), ignorestorage);
						stack.Pop(BooleanVariable::GetStorageSize());
					}
					break;
				case EpochVariableType_String:
					{
						StringVariable var(stack.GetCurrentTopOfStack());
						structureret->AddMember(*iter, var.GetAsRValue());
						structurevar.WriteMember(*iter, structureret->GetValue(*iter), ignorestorage);
						stack.Pop(StringVariable::GetStorageSize());
					}
					break;
				case EpochVariableType_Real:
					{
						RealVariable var(stack.GetCurrentTopOfStack());
						structureret->AddMember(*iter, var.GetAsRValue());
						structurevar.WriteMember(*iter, structureret->GetValue(*iter), ignorestorage);
						stack.Pop(RealVariable::GetStorageSize());
					}
					break;
				case EpochVariableType_Structure:
					{
						StructureVariable var(stack.GetCurrentTopOfStack());
						IDType nestedid = var.GetValue();
						RValuePtr p(PopVariableOffStack(*iter, var, stack, ignorestorage));
						structureret->AddMember(*iter, RValuePtr(p->Clone()));
						structurevar.WriteMember(*iter, p, ignorestorage);
					}
					break;
				case EpochVariableType_Function:
					{
						FunctionBinding var(stack.GetCurrentTopOfStack());
						structureret->AddMember(*iter, var.GetAsRValue());
						structurevar.WriteMember(*iter, structureret->GetValue(*iter), ignorestorage);
						stack.Pop(FunctionBinding::GetStorageSize());
					}
					break;
				case EpochVariableType_Buffer:
					{
						BufferVariable var(stack.GetCurrentTopOfStack());
						structureret->AddMember(*iter, var.GetAsRValue());
						structurevar.WriteMember(*iter, structureret->GetValue(*iter), ignorestorage);
						stack.Pop(BufferVariable::GetStorageSize());
					}
					break;
				default:
					throw NotImplementedException("Cannot read structure member of this type");
				}
			}

			return structureretptr;
		}
	case EpochVariableType_Buffer:
		{
			BufferVariable temp(stack.GetCurrentTopOfStack());
			RValuePtr ret(temp.GetAsRValue());
			var.CastTo<BufferVariable>().SetHandleValue(temp.GetHandleValue());
			stack.Pop(BufferVariable::GetStorageSize());
			return ret;
		}
	case EpochVariableType_Array:
		{
			ArrayVariable temp(stack.GetCurrentTopOfStack());
			RValuePtr ret(temp.GetAsRValue());
			var.CastTo<ArrayVariable>().SetValue(temp.GetValue());
			stack.Pop(ArrayVariable::GetBaseStorageSize());
			return ret;
		}
	default:
		throw NotImplementedException("Cannot pop variable of this type off the stack");
	}
}


//-------------------------------------------------------------------------------
// Ghosts
//-------------------------------------------------------------------------------

//
// Add linkage binding variables in the "other" scope to actual stored
// variables in this scope. The other scope will recognize the ghosted
// variables as valid, and defer to this scope to retrieve information
// about those variables.
//
// This strategy is used to handle function parameter and return value
// scopes. Since the two scopes are separate from the function body's
// outermost lexical scope, there needs to be a way to register their
// contents with the function's lexical scope so that the code within
// the function can access the parameter/return variables. Ghosting is
// our current solution for this issue.
//
void ActivatedScope::GhostIntoScope(ActivatedScope& other) const
{
	if(other.Ghosts.empty())
		throw InternalFailureException("Must PushNewGhostSet() before calling GhostIntoScope()");

	for(VariableMap::const_iterator iter = Variables.begin(); iter != Variables.end(); ++iter)
		other.Ghosts.back()[iter->first] = const_cast<ActivatedScope*>(this);

	for(VariableRefMap::const_iterator iter = References.begin(); iter != References.end(); ++iter)
		other.Ghosts.back()[iter->first] = const_cast<ActivatedScope*>(this);

	for(FunctionMap::const_iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
		other.Ghosts.back()[iter->first] = const_cast<ActivatedScope*>(this);
}

//
// Track a new set of ghosts for the current scope. We need to allow
// multiple layers of "current" ghosts in order to properly handle
// recursion and other re-entrant behaviours.
//
void ActivatedScope::PushNewGhostSet()
{
	Ghosts.push_back(GhostVariableMap());
}

//
// Note that a set of ghost links is no longer in use, and restore
// the ghost set (if any) that was active previously.
//
void ActivatedScope::PopGhostSet()
{
	Ghosts.pop_back();
}



//-------------------------------------------------------------------------------
// Variable getters and setters
//-------------------------------------------------------------------------------

//
// Retrieve the value of the given variable as an r-value
//
RValuePtr ActivatedScope::GetVariableValue(const std::wstring& name) const
{
	FutureMap::const_iterator futiter = Futures.find(name);
	if(futiter != Futures.end())
		return RValuePtr(futiter->second->GetValue());

	const Variable& var = LookupVariable(name);

	switch(var.GetType())
	{
	case EpochVariableType_Null:		return RValuePtr(new NullRValue());
	case EpochVariableType_Integer:		return GetVariableRef<IntegerVariable>(name).GetAsRValue();
	case EpochVariableType_Integer16:	return GetVariableRef<Integer16Variable>(name).GetAsRValue();
	case EpochVariableType_Real:		return GetVariableRef<RealVariable>(name).GetAsRValue();
	case EpochVariableType_Boolean:		return GetVariableRef<BooleanVariable>(name).GetAsRValue();
	case EpochVariableType_String:		return GetVariableRef<StringVariable>(name).GetAsRValue();
	case EpochVariableType_Tuple:
		{
			const TupleVariable& vartuple = var.CastTo<TupleVariable>();
			const TupleType& type = GetTupleType(vartuple.GetValue());
			RValuePtr retptr(new TupleRValue(type, vartuple.GetValue()));

			const std::vector<std::wstring>& members = type.GetMemberOrder();
			for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				retptr.get()->CastTo<TupleRValue>().AddMember(*iter, vartuple.ReadMember(*iter));

			return retptr;
		}
	case EpochVariableType_Structure:
		{
			const StructureVariable& varstruct = var.CastTo<StructureVariable>();
			const StructureType& type = GetStructureType(varstruct.GetValue());
			RValuePtr retptr(new StructureRValue(type, varstruct.GetValue()));

			const std::vector<std::wstring>& members = type.GetMemberOrder();
			for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				retptr.get()->CastTo<StructureRValue>().AddMember(*iter, RValuePtr(varstruct.ReadMember(*iter)->Clone()));

			return retptr;
		}
	case EpochVariableType_Buffer:		return GetVariableRef<BufferVariable>(name).GetAsRValue();
	case EpochVariableType_Array:		return GetVariableRef<ArrayVariable>(name).GetAsRValue();
	}

	throw NotImplementedException("Cannot retrieve variable value - unrecognized type");
}

//
// Set the value of the given variable to the provided r-value
//
RValuePtr ActivatedScope::SetVariableValue(const std::wstring& name, RValuePtr value)
{
	Variable& var = LookupVariable(name);

	if(var.GetType() != value->GetType() && value->GetType() != EpochVariableType_Null)
		throw ExecutionException("Cannot assign value to variable of different type");


	switch(value->GetType())
	{
	case EpochVariableType_Null:
		{
			if(var.GetType() != EpochVariableType_Null)
				throw ExecutionException("Cannot assign null to a variable");
		}
		break;

	case EpochVariableType_Integer:
		var.CastTo<IntegerVariable>().SetValue(value->CastTo<IntegerRValue>().GetValue());
		break;

	case EpochVariableType_Integer16:
		var.CastTo<Integer16Variable>().SetValue(value->CastTo<Integer16RValue>().GetValue());

	case EpochVariableType_String:
		var.CastTo<StringVariable>().SetValue(value->CastTo<StringRValue>().GetValue(), false);
		break;

	case EpochVariableType_Boolean:
		var.CastTo<BooleanVariable>().SetValue(value->CastTo<BooleanRValue>().GetValue());
		break;

	case EpochVariableType_Real:
		var.CastTo<RealVariable>().SetValue(value->CastTo<RealRValue>().GetValue());
		break;

	case EpochVariableType_Buffer:
		var.CastTo<BufferVariable>().SetHandleValue(value->CastTo<BufferRValue>().GetOriginHandle());
		break;

	case EpochVariableType_Array:
		value->CastTo<ArrayRValue>().StoreIntoNewBuffer();
		var.CastTo<ArrayVariable>().SetValue(value->CastTo<ArrayRValue>().GetHandle());
		break;

	default:
		throw NotImplementedException("Cannot set variable value for this type");
		break;
	}

	return RValuePtr(value->Clone());
}


//-------------------------------------------------------------------------------
// Passthroughs for obtaining information from the original scope description
//-------------------------------------------------------------------------------

EpochVariableTypeID ActivatedScope::GetVariableType(const std::wstring& name) const
{
	return OriginalScope.GetVariableType(name);
}

EpochVariableTypeID ActivatedScope::GetVariableType(unsigned memberindex) const
{
	return OriginalScope.GetVariableType(memberindex);
}

bool ActivatedScope::IsReference(const std::wstring& name) const
{
	return OriginalScope.IsReference(name);
}

bool ActivatedScope::IsReference(unsigned memberindex) const
{
	return OriginalScope.IsReference(memberindex);
}

bool ActivatedScope::HasTupleType(const std::wstring& name)
{
	return OriginalScope.HasTupleType(name);
}

const TupleType& ActivatedScope::GetTupleType(const std::wstring& name) const
{
	return OriginalScope.GetTupleType(name);
}

const TupleType& ActivatedScope::GetTupleType(IDType tupletypeid) const
{
	return OriginalScope.GetTupleType(tupletypeid);
}

IDType ActivatedScope::GetTupleTypeID(const std::wstring& name) const
{
	return OriginalScope.GetTupleTypeID(name);
}

const std::wstring& ActivatedScope::GetTupleTypeID(IDType id) const
{
	return OriginalScope.GetTupleTypeID(id);
}

IDType ActivatedScope::GetVariableTupleTypeID(const std::wstring& varname) const
{
	return OriginalScope.GetVariableTupleTypeID(varname);
}

IDType ActivatedScope::GetVariableTupleTypeID(unsigned memberindex) const
{
	return OriginalScope.GetVariableTupleTypeID(memberindex);
}


bool ActivatedScope::HasStructureType(const std::wstring& name) const
{
	return OriginalScope.HasStructureType(name);
}

const StructureType& ActivatedScope::GetStructureType(const std::wstring& name) const
{
	return OriginalScope.GetStructureType(name);
}

const StructureType& ActivatedScope::GetStructureType(IDType structuretypeid) const
{
	return OriginalScope.GetStructureType(structuretypeid);
}

IDType ActivatedScope::GetStructureTypeID(const std::wstring& name) const
{
	return OriginalScope.GetStructureTypeID(name);
}

const std::wstring& ActivatedScope::GetStructureTypeID(IDType id) const
{
	return OriginalScope.GetStructureTypeID(id);
}

IDType ActivatedScope::GetVariableStructureTypeID(const std::wstring& varname) const
{
	return OriginalScope.GetVariableStructureTypeID(varname);
}

IDType ActivatedScope::GetVariableStructureTypeID(unsigned memberindex) const
{
	return OriginalScope.GetVariableStructureTypeID(memberindex);
}


bool ActivatedScope::IsFunctionSignature(const std::wstring& name) const
{
	return OriginalScope.IsFunctionSignature(name);
}

bool ActivatedScope::IsFunctionSignature(unsigned memberindex) const
{
	return OriginalScope.IsFunctionSignature(memberindex);
}


const FunctionSignature& ActivatedScope::GetFunctionSignature(const std::wstring& name) const
{
	return OriginalScope.GetFunctionSignature(name);
}

const FunctionSignature& ActivatedScope::GetFunctionSignature(unsigned memberindex) const
{
	return OriginalScope.GetFunctionSignature(memberindex);
}



//-------------------------------------------------------------------------------
// Futures
//-------------------------------------------------------------------------------

//
// Retrieve a future from the scope
//
Future* ActivatedScope::GetFuture(const std::wstring& name)
{
	FutureMap::iterator iter = Futures.find(name);
	if(iter == Futures.end())
	{
		if(ParentScope)
			return ParentScope->GetFuture(name);

		throw ExecutionException("Future not found!");
	}

	return iter->second;
}



//-------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------

//
// Add a function to the scope
//
// Usually functions are added to the ScopeDescription template
// at compile time; however, in order to facilitate dynamic
// binding of functions via function reference variables, we
// need the ability to register a function at runtime as well.
//
void ActivatedScope::AddFunction(const std::wstring& name, FunctionBase* func)
{
	CheckForDuplicateIdentifier(name);
	Functions.insert(FunctionMapEntry(name, func));
}

//
// Retrieve a function or runtime-bound function given its name
//
FunctionBase* ActivatedScope::GetFunction(const std::wstring& name) const
{
	FunctionMap::const_iterator iter = Functions.find(name);
	if(iter != Functions.end())
		return iter->second;

	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(name);
		if(ghostiter != Ghosts.back().end())
			return ghostiter->second->GetFunction(name);
	}

	if(ParentScope)
		return ParentScope->GetFunction(name);

	return OriginalScope.GetFunction(name);
}



//-------------------------------------------------------------------------------
// Helpers for multiple return values
//-------------------------------------------------------------------------------

//
// Retrieve the contents of the scope's variables in a tuple.
// This tuple is used for returning multiple function values.
//
RValuePtr ActivatedScope::GetEffectiveTuple() const
{
	if(Variables.empty())
		return RValuePtr(new NullRValue());

	if(Variables.size() == 1)
		return GetVariableValue(Variables.begin()->first);

	IDType id = TupleTrackerClass::InvalidID;
	
	const ActivatedScope* scope = this;
	do
	{
		id = TupleTrackerClass::LookForMatchingTupleType(scope->OriginalScope);
		if(id == TupleTrackerClass::InvalidID)
			scope = scope->ParentScope;
		else
			break;
	} while(scope);

	if(id == TupleTrackerClass::InvalidID)
		throw InternalFailureException("Failed to find a suitable tuple type; something is probably broken in the parser");

	return RValuePtr(new TupleRValue(OriginalScope.TupleTracker.GetTupleType(id), *this, id));
}



//-------------------------------------------------------------------------------
// Internal helpers
//-------------------------------------------------------------------------------

//
// Look up and retrieve a reference to the variable with the given name
//
Variable& ActivatedScope::LookupVariable(const std::wstring& name) const
{
	VariableMap::const_iterator iter = Variables.find(name);
	if(iter != Variables.end())
		return const_cast<Variable&>(iter->second);

	if(!Ghosts.empty())
	{
		GhostVariableMap::const_iterator ghostiter = Ghosts.back().find(name);
		if(ghostiter != Ghosts.back().end())
			return ghostiter->second->LookupVariable(name);
	}

	VariableRefMap::const_iterator refiter = References.find(name);
	if(refiter != References.end())
	{
		if(!refiter->second.second)
			throw ExecutionException("Cannot access unbound reference");
		return *refiter->second.second;
	}

	if(ParentScope)
		return ParentScope->LookupVariable(name);

	throw MissingVariableException("Failed to find the given variable");
}

//
// Ensure that we cannot duplicate identifiers within scopes, or introduce shadowing identifiers
//
void ActivatedScope::CheckForDuplicateIdentifier(const std::wstring& name) const
{
	if(Variables.find(name) != Variables.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a variable identifier");

	if(References.find(name) != References.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a variable reference identifier");

	if(Futures.find(name) != Futures.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a future identifier");

	if(Functions.find(name) != Functions.end())
		throw DuplicateIdentifierException("The name \"" + narrow(name) + "\" is already in use as a function identifier");

	for(std::deque<GhostVariableMap>::const_iterator iter = Ghosts.begin(); iter != Ghosts.end(); ++iter)
	{
		GhostVariableMap::const_iterator ghostiter = iter->find(name);
		if(ghostiter != iter->end())
			ghostiter->second->CheckForDuplicateIdentifier(name);
	}

	if(ParentScope)
		ParentScope->CheckForDuplicateIdentifier(name);
}


//-------------------------------------------------------------------------------
// Task related information
//-------------------------------------------------------------------------------

//
// Locate the ID of the task that forked this task; if this
// is the root task, returns 0.
//
unsigned ActivatedScope::FindTaskOrigin() const
{
	if(TaskOrigin)
		return TaskOrigin;

	if(ParentScope)
		return ParentScope->FindTaskOrigin();

	return 0;
}

//
// Locate the sender of the last received message by this task;
// if no messages have been received, returns 0.
//
unsigned ActivatedScope::FindLastMessageOrigin() const
{
	if(LastMessageOrigin)
		return LastMessageOrigin;

	if(ParentScope)
		return ParentScope->FindLastMessageOrigin();

	return 0;
}


