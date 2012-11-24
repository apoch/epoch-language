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


namespace
{
	void* ScopeStack = NULL;
	void* CurrentAlloc = NULL;


	void* DataStack = NULL;
	void* CurrentDataAlloc = NULL;


	void* StackAlloc(size_t bytes)
	{
		void* ret = CurrentDataAlloc;
		CurrentDataAlloc = reinterpret_cast<char*>(CurrentDataAlloc) + bytes;
		return ret;
	}

	void StackFree(size_t bytes)
	{
		CurrentDataAlloc = reinterpret_cast<char*>(CurrentDataAlloc) - bytes;
	}
}



void ActiveScope::InitAllocator()
{
	if(ScopeStack)
		::VirtualFree(ScopeStack, 0, MEM_RELEASE);
	if(DataStack)
		::VirtualFree(DataStack, 0, MEM_RELEASE);

	ScopeStack = ::VirtualAlloc(0, 1024 * sizeof(ActiveScope), MEM_COMMIT, PAGE_READWRITE);
	CurrentAlloc = ScopeStack;

	DataStack = ::VirtualAlloc(0, 1024 * 1024 * 5, MEM_COMMIT, PAGE_READWRITE);
	CurrentDataAlloc = DataStack;
}


void* ActiveScope::operator new(size_t size)
{
	void* ret = CurrentAlloc;
	CurrentAlloc = reinterpret_cast<char*>(CurrentAlloc) + size;
	return ret;
}

void ActiveScope::operator delete(void* p)
{
	CurrentAlloc = reinterpret_cast<char*>(CurrentAlloc) - sizeof(ActiveScope);

	if(p != CurrentAlloc)
		std::terminate();		// can't throw here!
}


ActiveScope::ActiveScope(const ScopeDescription& originalscope, ActiveScope* parent)
	: OriginalScope(originalscope),
	  ParentScope(parent),
	  StartOfLocals(NULL),
	  StartOfParams(NULL),
	  UsedStackSpace(0),
	  Data(NULL),
	  HasRet(false)
{
	DataSize = originalscope.Variables.size() * sizeof(RuntimeData);
	if(DataSize)
		Data = reinterpret_cast<RuntimeData*>(StackAlloc(DataSize));
}

ActiveScope::~ActiveScope()
{
	StackFree(DataSize);
}


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
	StartOfParams = stackpointer;

	size_t i = OriginalScope.Variables.size();
	for(ScopeDescription::VariableVector::const_reverse_iterator iter = OriginalScope.Variables.rbegin(); iter != OriginalScope.Variables.rend(); ++iter)
	{
		--i;
		if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
		{
			if(iter->IsReference)
			{
				void* targetstorage = *reinterpret_cast<void**>(stackpointer);
				stackpointer += sizeof(void*);
				Metadata::EpochTypeID targettype = *reinterpret_cast<Metadata::EpochTypeID*>(stackpointer);
				stackpointer += sizeof(Metadata::EpochTypeID);
				Data[i].RefInfo.first = targetstorage;
				Data[i].RefInfo.second = targettype;
			}
			else
			{				
				if(Metadata::GetTypeFamily(iter->Type) == Metadata::EpochTypeFamily_SumType)
				{
					Data[i].ActualType = *reinterpret_cast<Metadata::EpochTypeID*>(stackpointer);
					stackpointer += sizeof(Metadata::EpochTypeID);
					Data[i].StorageLocation = stackpointer;
					stackpointer += Metadata::GetStorageSize(Data[i].ActualType);
				}
				else
				{
					Data[i].StorageLocation = stackpointer;
					stackpointer += Metadata::GetStorageSize(iter->Type);
				}
			}
		}
	}

	UsedStackSpace += (reinterpret_cast<char*>(stackpointer) - reinterpret_cast<char*>(StartOfParams));
}

//
// Allocate stack space for all local variables (including return value holders) on the given stack
//
void ActiveScope::PushLocalsOntoStack(VM::ExecutionContext& context)
{
	StackSpace& stack = context.State.Stack;
	StartOfLocals = stack.GetCurrentTopOfStack();

	size_t varcount = OriginalScope.Variables.size();
	RuntimeData* data = &Data[0];
	for(size_t i = 0; i < varcount; ++i)
	{
		const ScopeDescription::VariableEntry& v = OriginalScope.Variables[i];

		if(v.Origin == VARIABLE_ORIGIN_LOCAL || v.Origin == VARIABLE_ORIGIN_RETURN)
		{
			if(Metadata::GetTypeFamily(v.Type) == Metadata::EpochTypeFamily_SumType)
			{
				size_t size = context.OwnerVM.VariantDefinitions[v.Type].GetMaxSize();
				stack.Push(size);

				data->StorageLocation = reinterpret_cast<char*>(stack.GetCurrentTopOfStack()) + sizeof(Metadata::EpochTypeID);
			}
			else
			{
				size_t size = Metadata::GetStorageSize(v.Type);
				stack.Push(size);

				data->StorageLocation = stack.GetCurrentTopOfStack();
			}
		}

		if(v.Origin == VARIABLE_ORIGIN_RETURN)
			HasRet = true;

		++data;
	}

	UsedStackSpace += reinterpret_cast<char*>(StartOfLocals) - reinterpret_cast<char*>(stack.GetCurrentTopOfStack());
}

//
// Pop a stack so as to reset it after holding parameters and local variables from the current scope
//
void ActiveScope::PopScopeOffStack(VM::ExecutionContext& context)
{
	context.State.Stack.Pop(UsedStackSpace);
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
		for(size_t i = 0; i < OriginalScope.Variables.size(); ++i)
		{
			if(OriginalScope.Variables[i].IdentifierHandle == variableid)
			{
				Metadata::EpochTypeID actualtype = Data[i].ActualType;

				PushOntoStack(Data[i].StorageLocation, actualtype, stack);
				stack.PushValue(actualtype);
				return;
			}
		}

		//
		// This is an internal failure of the VM to maintain type information.
		//
		throw FatalException("Missing actual type for sum typed variable in local scope");
	}
	else
		PushOntoStack(GetVariableStorageLocation(variableid), vartype, stack);
}

//
// Dereference a variable and push the resultant value onto the stack
//
void ActiveScope::PushOntoStackDeref(StringHandle variableid, StackSpace& stack) const
{
	for(size_t i = 0; i < OriginalScope.Variables.size(); ++i)
	{
		if(OriginalScope.Variables[i].IdentifierHandle == variableid)
		{
			PushOntoStack(Data[i].RefInfo.first, Data[i].RefInfo.second, stack);
			return;
		}
	}

	throw FatalException("Unbound reference");
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
		for(size_t i = 0; i < thisptr->OriginalScope.Variables.size(); ++i)
		{
			if(thisptr->OriginalScope.Variables[i].IdentifierHandle == variableid && thisptr->Data[i].StorageLocation)
				return thisptr->Data[i].StorageLocation;
		}

		thisptr = thisptr->ParentScope;
	}

	throw InvalidIdentifierException("Variable ID has not been mapped to a storage location in this scope");
}

//
// Copy a variable's value into the provided virtual machine register
//
void ActiveScope::CopyToRegister(size_t index, Register& targetregister) const
{
	targetregister.SumType = false;

	Metadata::EpochTypeID variabletype = OriginalScope.Variables[index].Type;
	switch(variabletype)
	{
	case Metadata::EpochType_Integer:
		{
			Integer32* value = reinterpret_cast<Integer32*>(Data[index].StorageLocation);
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Integer16:
		{
			Integer16* value = reinterpret_cast<Integer16*>(Data[index].StorageLocation);
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Identifier:
	case Metadata::EpochType_String:
		{
			StringHandle* value = reinterpret_cast<StringHandle*>(Data[index].StorageLocation);
			targetregister.SetString(*value);
		}
		break;

	case Metadata::EpochType_Boolean:
		{
			bool* value = reinterpret_cast<bool*>(Data[index].StorageLocation);
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Real:
		{
			Real32* value = reinterpret_cast<Real32*>(Data[index].StorageLocation);
			targetregister.Set(*value);
		}
		break;

	case Metadata::EpochType_Buffer:
		{
			BufferHandle* value = reinterpret_cast<BufferHandle*>(Data[index].StorageLocation);
			targetregister.SetBuffer(*value);
		}
		break;

	default:
		{
			if(Metadata::GetTypeFamily(variabletype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(variabletype) == Metadata::EpochTypeFamily_TemplateInstance)
			{
				StructureHandle* value = reinterpret_cast<StructureHandle*>(Data[index].StorageLocation);
				targetregister.SetStructure(*value, variabletype);
			}
			else if(Metadata::GetTypeFamily(variabletype) == Metadata::EpochTypeFamily_SumType)
			{
				const UByte* storage = reinterpret_cast<const UByte*>(Data[index].StorageLocation);
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
// Retrieve the storage location pointed to by a reference variable
//
void* ActiveScope::GetReferenceTarget(size_t index) const
{
	return Data[index].RefInfo.first;
}

void* ActiveScope::GetReferenceTargetByName(StringHandle name) const
{
	for(size_t i = 0; i < OriginalScope.Variables.size(); ++i)
	{
		if(OriginalScope.Variables[i].IdentifierHandle == name)
			return Data[i].RefInfo.first;
	}

	if(ParentScope)
		return ParentScope->GetReferenceTargetByName(name);

	throw FatalException("Unbound reference");
}

//
// Retrieve the underlying type of a reference variable
//
Metadata::EpochTypeID ActiveScope::GetReferenceType(size_t index) const
{
	return Data[index].RefInfo.second;
}

Metadata::EpochTypeID ActiveScope::GetActualType(size_t varindex) const
{
	Metadata::EpochTypeID ret = OriginalScope.GetVariableTypeByIndex(varindex);
	if(Metadata::GetTypeFamily(ret) == Metadata::EpochTypeFamily_SumType)
	{
		const char* ptr = reinterpret_cast<const char*>(Data[varindex].StorageLocation);
		ptr -= sizeof(Metadata::EpochTypeID);
		ret = *reinterpret_cast<const Metadata::EpochTypeID*>(ptr);
	}

	return ret;
}


void ActiveScope::SetActualType(StringHandle varname, Metadata::EpochTypeID type)
{
	for(size_t i = 0; i < OriginalScope.Variables.size(); ++i)
	{
		if(OriginalScope.Variables[i].IdentifierHandle == varname)
		{
			Data[i].ActualType = type;
			return;
		}
	}

	throw FatalException("Cannot set sum type annotation in this context");
}

