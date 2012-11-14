//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for containing a lexical scope and its contents
//

#include "pch.h"

#include "Metadata/ActiveScope.h"
#include "Metadata/ScopeDescription.h"
#include "Metadata/TypeInfo.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Utility/Memory/Stack.h"

#include "Utility/Types/RealTypes.h"


//
// Attach the parameters defined in the current scope to a given stack space
//
// Each variable within the scope which has its origin specified as an entity parameter is
// bound to a storage location within the given stack space. The stack space is assumed to
// contain all of the parameters to the entity in order as per the calling convention.
//
void ActiveScope::BindParametersToStack(const VM::ExecutionContext& context)
{
	char* stackpointer = reinterpret_cast<char*>(context.State.Stack.GetCurrentTopOfStack());

	for(ScopeDescription::VariableVector::const_reverse_iterator iter = OriginalScope.Variables.rbegin(); iter != OriginalScope.Variables.rend(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
		{
			if(iter->IsReference)
			{
				void* targetstorage = *reinterpret_cast<void**>(stackpointer);
				stackpointer += sizeof(void*);
				Metadata::EpochTypeID targettype = *reinterpret_cast<Metadata::EpochTypeID*>(stackpointer);
				stackpointer += sizeof(Metadata::EpochTypeID);
				BindReference(iter->IdentifierHandle, targetstorage, targettype);
			}
			else
			{				
				if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_SumType)
				{
					ActualTypes[iter->IdentifierHandle] = *reinterpret_cast<Metadata::EpochTypeID*>(stackpointer);
					stackpointer += sizeof(Metadata::EpochTypeID);
					VariableStorageLocations[iter->IdentifierHandle] = stackpointer;
					stackpointer += Metadata::GetStorageSize(ActualTypes[iter->IdentifierHandle]);
				}
				else
				{
					VariableStorageLocations[iter->IdentifierHandle] = stackpointer;
					stackpointer += Metadata::GetStorageSize(iter->Type);
				}
			}
		}
	}
}

//
// Allocate stack space for all local variables (including return value holders) on the given stack
//
void ActiveScope::PushLocalsOntoStack(VM::ExecutionContext& context)
{
	for(ScopeDescription::VariableVector::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_LOCAL || iter->Origin == VARIABLE_ORIGIN_RETURN)
		{
			if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_SumType)
			{
				size_t size = context.OwnerVM.VariantDefinitions.find(iter->Type)->second.GetMaxSize();
				context.State.Stack.Push(size);

				VariableStorageLocations[iter->IdentifierHandle] = reinterpret_cast<char*>(context.State.Stack.GetCurrentTopOfStack()) + sizeof(Metadata::EpochTypeID);
			}
			else
			{
				size_t size = Metadata::GetStorageSize(iter->Type);
				context.State.Stack.Push(size);

				VariableStorageLocations[iter->IdentifierHandle] = context.State.Stack.GetCurrentTopOfStack();
			}
		}
	}
}

//
// Pop a stack so as to reset it after holding parameters and local variables from the current scope
//
void ActiveScope::PopScopeOffStack(VM::ExecutionContext& context)
{
	size_t usedspace = 0;

	for(ScopeDescription::VariableVector::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
	{
		if(iter->IsReference)
			usedspace += sizeof(Metadata::EpochTypeID) + sizeof(void*);
		else
		{
			if(iter->Origin == VARIABLE_ORIGIN_PARAMETER && ActualTypes.find(iter->IdentifierHandle) != ActualTypes.end())
			{
				usedspace += Metadata::GetStorageSize(ActualTypes.find(iter->IdentifierHandle)->second);
				usedspace += sizeof(Metadata::EpochTypeID);
			}
			else if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_SumType)
			{
				usedspace += context.OwnerVM.VariantDefinitions.find(iter->Type)->second.GetMaxSize();

				if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
					usedspace += sizeof(Metadata::EpochTypeID);
			}
			else
				usedspace += Metadata::GetStorageSize(iter->Type);
		}
	}

	context.State.Stack.Pop(usedspace);
}

//
// Write the topmost stack entry into a given arbitrary storage location
//
void ActiveScope::WriteFromStack(void* targetstorage, Metadata::EpochTypeID targettype, StackSpace& stack)
{
	switch(targettype)
	{
	case Metadata::EpochType_Integer:
		Write(targetstorage, stack.PopValue<Integer32>());
		break;

	case Metadata::EpochType_Integer16:
		Write(targetstorage, stack.PopValue<Integer16>());
		break;

	case Metadata::EpochType_String:
		Write(targetstorage, stack.PopValue<StringHandle>());
		break;

	case Metadata::EpochType_Boolean:
		Write(targetstorage, stack.PopValue<bool>());
		break;

	case Metadata::EpochType_Real:
		Write(targetstorage, stack.PopValue<Real32>());
		break;

	case Metadata::EpochType_Buffer:
		Write(targetstorage, stack.PopValue<BufferHandle>());
		break;

	case Metadata::EpochType_Nothing:
		break;

	default:
		if(Metadata::GetTypeFamily(targettype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(targettype) == Metadata::EpochTypeFamily_TemplateInstance)
			Write(targetstorage, stack.PopValue<StructureHandle>());
		else if(Metadata::GetTypeFamily(targettype) == Metadata::EpochTypeFamily_SumType)
		{
			Metadata::EpochTypeID actualtype = stack.PopValue<Metadata::EpochTypeID>();
			WriteFromStack(targetstorage, actualtype, stack);
			char* storageptr = reinterpret_cast<char*>(targetstorage);
			storageptr -= sizeof(Metadata::EpochTypeID);
			*reinterpret_cast<Metadata::EpochTypeID*>(storageptr) = actualtype;
		}
		else
			throw NotImplementedException("Unsupported type in ActiveScope::WriteFromStack");
		break;
	}
}


//
// Internal helper to minimize code redundancy
//
namespace
{
	template <typename T>
	void DoTypedPush(StackSpace& stack, void* targetstorage)
	{
		T* value = reinterpret_cast<T*>(targetstorage);
		stack.PushValue<T>(*value);
	}
}


//
// Push data from an arbitrary location onto the stack
//
void ActiveScope::PushOntoStack(void* targetstorage, Metadata::EpochTypeID targettype, StackSpace& stack) const
{
	switch(targettype)
	{
	case Metadata::EpochType_Integer:			DoTypedPush<Integer32>(stack, targetstorage);		break;
	case Metadata::EpochType_Integer16:		DoTypedPush<Integer16>(stack, targetstorage);		break;
	case Metadata::EpochType_String:			DoTypedPush<StringHandle>(stack, targetstorage);	break;
	case Metadata::EpochType_Boolean:			DoTypedPush<bool>(stack, targetstorage);			break;
	case Metadata::EpochType_Real:			DoTypedPush<Real32>(stack, targetstorage);			break;
	case Metadata::EpochType_Buffer:			DoTypedPush<BufferHandle>(stack, targetstorage);	break;
	case Metadata::EpochType_Function:		DoTypedPush<StringHandle>(stack, targetstorage);	break;
	case Metadata::EpochType_Identifier:		DoTypedPush<StringHandle>(stack, targetstorage);	break;
	case Metadata::EpochType_Nothing:			stack.Push(0);										break;
	default:
		{
			Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(targettype);
			if(family != Metadata::EpochTypeFamily_Structure && family != Metadata::EpochTypeFamily_TemplateInstance)
				throw NotImplementedException("Unsupported data type in ActiveScope::PushOntoStack");
		}

		DoTypedPush<StructureHandle>(stack, targetstorage);
		break;
	}
}

//
// Push a variable's value onto the top of the given stack
//
void ActiveScope::PushOntoStack(StringHandle variableid, StackSpace& stack) const
{
	if(OriginalScope.IsReferenceByID(variableid))
	{
		PushOntoStackDeref(variableid, stack);
		return;
	}

	Metadata::EpochTypeID vartype = OriginalScope.GetVariableTypeByID(variableid);
	if(Metadata::GetTypeFamily(vartype) == Metadata::EpochTypeFamily_SumType)
	{
		std::map<StringHandle, Metadata::EpochTypeID>::const_iterator iter = ActualTypes.find(variableid);
		if(iter == ActualTypes.end())
		{
			//
			// This is an internal failure of the VM to maintain type information.
			//
			throw FatalException("Missing actual type for sum typed variable in local scope");
		}

		Metadata::EpochTypeID actualtype = iter->second;

		PushOntoStack(GetVariableStorageLocation(variableid), actualtype, stack);
		stack.PushValue(actualtype);
	}
	else
		PushOntoStack(GetVariableStorageLocation(variableid), vartype, stack);
}

//
// Dereference a variable and push the resultant value onto the stack
//
void ActiveScope::PushOntoStackDeref(StringHandle variableid, StackSpace& stack) const
{
	ReferenceBindingMap::const_iterator iter = BoundReferences.find(variableid);
	if(iter == BoundReferences.end())
		throw FatalException("Unbound reference");

	PushOntoStack(iter->second.first, iter->second.second, stack);
}

//
// Retrieve the storage location in memory of the given variable
//
// This may reside on the stack or the freestore or even be a reference to another
// variable's storage position, depending on the context. In general it is not safe
// to assume that a variable resides in any one particular location.
//
void* ActiveScope::GetVariableStorageLocation(StringHandle variableid) const
{
	const ActiveScope* thisptr = this;
	while(thisptr)
	{
		std::map<StringHandle, void*>::const_iterator iter = thisptr->VariableStorageLocations.find(variableid);
		if(iter != thisptr->VariableStorageLocations.end())
			return iter->second;

		thisptr = thisptr->ParentScope;
	}

	throw InvalidIdentifierException("Variable ID has not been mapped to a storage location in this scope");
}

//
// Copy a variable's value into the provided virtual machine register
//
void ActiveScope::CopyToRegister(StringHandle variableid, Register& targetregister) const
{
	targetregister.SumType = false;

	Metadata::EpochTypeID variabletype = OriginalScope.GetVariableTypeByID(variableid);
	switch(variabletype)
	{
	case Metadata::EpochType_Integer:
		{
			Integer32* value = reinterpret_cast<Integer32*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Integer16:
		{
			Integer16* value = reinterpret_cast<Integer16*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Identifier:
	case Metadata::EpochType_String:
		{
			StringHandle* value = reinterpret_cast<StringHandle*>(GetVariableStorageLocation(variableid));
			targetregister.SetString(*value);
		}
		break;

	case Metadata::EpochType_Boolean:
		{
			bool* value = reinterpret_cast<bool*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Real:
		{
			Real32* value = reinterpret_cast<Real32*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Buffer:
		{
			BufferHandle* value = reinterpret_cast<BufferHandle*>(GetVariableStorageLocation(variableid));
			targetregister.SetBuffer(*value);
		}
		break;

	default:
		{
			if(Metadata::GetTypeFamily(variabletype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(variabletype) == Metadata::EpochTypeFamily_TemplateInstance)
			{
				StructureHandle* value = reinterpret_cast<StructureHandle*>(GetVariableStorageLocation(variableid));
				targetregister.SetStructure(*value, OriginalScope.GetVariableTypeByID(variableid));
			}
			else if(Metadata::GetTypeFamily(variabletype) == Metadata::EpochTypeFamily_SumType)
			{
				const UByte* storage = reinterpret_cast<const UByte*>(GetVariableStorageLocation(variableid));
				const UByte* typestorage = storage - sizeof(Metadata::EpochTypeID);
				targetregister.Type = *reinterpret_cast<const Metadata::EpochTypeID*>(typestorage);
				switch(targetregister.Type)
				{
				case Metadata::EpochType_Boolean:		targetregister.Value_Boolean = *reinterpret_cast<const bool*>(storage);						break;
				case Metadata::EpochType_Buffer:		targetregister.Value_BufferHandle = *reinterpret_cast<const BufferHandle*>(storage);		break;
				case Metadata::EpochType_String:
				case Metadata::EpochType_Identifier:	targetregister.Value_StringHandle = *reinterpret_cast<const StringHandle*>(storage);		break;
				case Metadata::EpochType_Integer:		targetregister.Value_Integer32 = *reinterpret_cast<const Integer32*>(storage);				break;
				case Metadata::EpochType_Integer16:	targetregister.Value_Integer16 = *reinterpret_cast<const Integer16*>(storage);				break;
				case Metadata::EpochType_Nothing:		break;
				case Metadata::EpochType_Real:		targetregister.Value_Real = *reinterpret_cast<const Real32*>(storage);						break;
				default:
					if(Metadata::GetTypeFamily(targetregister.Type) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(targetregister.Type) == Metadata::EpochTypeFamily_TemplateInstance)
						targetregister.Value_StructureHandle = *reinterpret_cast<const StructureHandle*>(storage);
					else
						throw FatalException("Unsupported actual type of sum type");

					break;
				}
				targetregister.SumType = true;
			}
			else
				throw FatalException("Unsupported type when assigning to register");
		}
		break;
	}
}

//
// Determine if the current scope contains a variable bound to an entity return value
//
bool ActiveScope::HasReturnVariable() const
{
	for(ScopeDescription::VariableVector::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_RETURN)
			return true;
	}

	return false;
}


//
// Bind a reference variable to a target
//
void ActiveScope::BindReference(StringHandle referencename, void* targetstorage, Metadata::EpochTypeID targettype)
{
	BoundReferences[referencename].first = targetstorage;
	BoundReferences[referencename].second = targettype;
}

//
// Retrieve the storage location pointed to by a reference variable
//
void* ActiveScope::GetReferenceTarget(StringHandle referencename) const
{
	ReferenceBindingMap::const_iterator iter = BoundReferences.find(referencename);
	if(iter == BoundReferences.end())
		throw FatalException("Unbound reference");

	return iter->second.first;
}

//
// Retrieve the underlying type of a reference variable
//
Metadata::EpochTypeID ActiveScope::GetReferenceType(StringHandle referencename) const
{
	ReferenceBindingMap::const_iterator iter = BoundReferences.find(referencename);
	if(iter == BoundReferences.end())
		throw FatalException("Unbound reference");

	return iter->second.second;
}

Metadata::EpochTypeID ActiveScope::GetActualType(StringHandle varname) const
{
	Metadata::EpochTypeID ret = OriginalScope.GetVariableTypeByID(varname);
	if(Metadata::GetTypeFamily(ret) == Metadata::EpochTypeFamily_SumType)
	{
		const char* ptr = reinterpret_cast<const char*>(GetVariableStorageLocation(varname));
		ptr -= sizeof(Metadata::EpochTypeID);
		ret = *reinterpret_cast<const Metadata::EpochTypeID*>(ptr);
	}

	return ret;
}


void ActiveScope::SetActualType(StringHandle varname, Metadata::EpochTypeID type)
{
	ActualTypes[varname] = type;
}

