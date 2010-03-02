//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for relaying messages between Epoch tasks
//

#include "pch.h"


#include "Virtual Machine/Operations/Concurrency/Messaging.h"

#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/SelfAware.inl"

#include "Virtual Machine/Routines.inl"

#include "Virtual Machine/Types Management/TypeInfo.h"

#include "Utility/Threading/Threads.h"


using namespace VM;
using namespace VM::Operations;


// Prototypes
namespace
{
	void Dispatch(const std::vector<ResponseMapEntry*>& mapentries, ExecutionContext& context, HandleType taskorigin);
}


//
// Send a message to another task
//
void SendTaskMessage::ExecuteFast(ExecutionContext& context)
{
	std::wstring targetname;
	TaskHandle threadid;

	if(UsesTaskID)
	{
		StringVariable temp(context.Stack.GetCurrentTopOfStack());
		targetname = temp.GetValue();
		context.Stack.Pop(temp.GetStorageSize());
	}
	else
	{
		TaskHandleVariable threadidvar(context.Stack.GetCurrentTopOfStack());
		threadid = threadidvar.GetValue();
		context.Stack.Pop(TaskHandleVariable::GetStorageSize());
		targetname = Threads::GetThreadNameGivenID(threadid);
	}

	std::auto_ptr<HeapStorage> heapblock(new HeapStorage);
	size_t neededstorage = 0;
	for(std::list<EpochVariableTypeID>::const_iterator iter = PayloadTypes.begin(); iter != PayloadTypes.end(); ++iter)
	{
		if(*iter == EpochVariableType_Array)
			neededstorage += sizeof(HandleType);
		else
			neededstorage += TypeInfo::GetStorageSize(*iter);
	}

	heapblock->Allocate(neededstorage);
	void* storageptr = heapblock->GetStartOfStorage();
	for(std::list<EpochVariableTypeID>::const_iterator iter = PayloadTypes.begin(); iter != PayloadTypes.end(); ++iter)
	{
		switch(*iter)
		{
		case EpochVariableType_Integer:
			{
				IntegerVariable var(context.Stack.GetCurrentTopOfStack());
				*reinterpret_cast<IntegerVariable::BaseStorage*>(storageptr) = var.GetValue();
				storageptr = reinterpret_cast<Byte*>(storageptr) + IntegerVariable::GetStorageSize();
				context.Stack.Pop(IntegerVariable::GetStorageSize());
			}
			break;

		case EpochVariableType_Integer16:
			{
				Integer16Variable var(context.Stack.GetCurrentTopOfStack());
				*reinterpret_cast<Integer16Variable::BaseStorage*>(storageptr) = var.GetValue();
				storageptr = reinterpret_cast<Byte*>(storageptr) + Integer16Variable::GetStorageSize();
				context.Stack.Pop(Integer16Variable::GetStorageSize());
			}
			break;

		case EpochVariableType_Real:
			{
				RealVariable var(context.Stack.GetCurrentTopOfStack());
				*reinterpret_cast<RealVariable::BaseStorage*>(storageptr) = var.GetValue();
				storageptr = reinterpret_cast<Byte*>(storageptr) + RealVariable::GetStorageSize();
				context.Stack.Pop(RealVariable::GetStorageSize());
			}
			break;

		case EpochVariableType_Boolean:
			{
				BooleanVariable var(context.Stack.GetCurrentTopOfStack());
				*reinterpret_cast<BooleanVariable::BaseStorage*>(storageptr) = var.GetValue();
				storageptr = reinterpret_cast<Byte*>(storageptr) + BooleanVariable::GetStorageSize();
				context.Stack.Pop(BooleanVariable::GetStorageSize());
			}
			break;

		case EpochVariableType_String:
			{
				StringVariable var(context.Stack.GetCurrentTopOfStack());
				*reinterpret_cast<StringVariable::BaseStorage*>(storageptr) = var.GetHandleValue();
				storageptr = reinterpret_cast<Byte*>(storageptr) + StringVariable::GetStorageSize();
				context.Stack.Pop(StringVariable::GetStorageSize());
			}
			break;

		case EpochVariableType_Array:
			{
				ArrayVariable var(context.Stack.GetCurrentTopOfStack());
				*reinterpret_cast<ArrayVariable::BaseStorage*>(storageptr) = var.GetValue();
				storageptr = reinterpret_cast<Byte*>(storageptr) + ArrayVariable::GetBaseStorageSize();
				context.Stack.Pop(ArrayVariable::GetBaseStorageSize());
			}
			break;

		default:
			throw NotImplementedException("Cannot pass this data type in a message payload");
		}
	}

	Threads::SendEvent(targetname, MessageName, PayloadTypes, heapblock.release());
}

RValuePtr SendTaskMessage::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}


//
// Construct and initialize an operation for accepting messages from other tasks
//
AcceptMessage::AcceptMessage(const std::wstring& messagename, Block* theblock, ScopeDescription* helperscope)
{
	std::list<EpochVariableTypeID> payloadtypes;
	const std::vector<std::wstring>& members = helperscope->GetMemberOrder();
	for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
		payloadtypes.push_back(helperscope->GetVariableType(*iter));

	ResponseEntry = new ResponseMapEntry(messagename, payloadtypes, theblock, helperscope);
}


//
// Destruct and clean up a message acceptance operation
//
AcceptMessage::~AcceptMessage()
{
	delete ResponseEntry;
}

//
// Accept an incoming message from another task, matching a specific signature
//
// Note that this blocks until a matching message is received. Also note that
// while this operation is active, any non-recognized messages will be ignored.
//
void AcceptMessage::ExecuteFast(ExecutionContext& context)
{
	std::vector<ResponseMapEntry*> vec;
	vec.push_back(ResponseEntry);

	Dispatch(vec, context, context.Scope.TaskOrigin);
}

RValuePtr AcceptMessage::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}

template <typename TraverserT>
void AcceptMessage::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	ResponseEntry->GetResponseBlock()->Traverse(traverser);
	traverser.RegisterScope(*ResponseEntry->GetHelperScope());
}

void AcceptMessage::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void AcceptMessage::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

const std::wstring& AcceptMessage::GetMessageName() const
{
	return ResponseEntry->GetMessageName();
}

const std::list<VM::EpochVariableTypeID>& AcceptMessage::GetPayloadTypes() const
{
	return ResponseEntry->GetPayloadTypes();
}


//
// Wait for a message that matches one of several patterns in a response map.
//
// Note that this operation blocks until a matching message is located.
// Also note that while this operation is active, non-matched messages
// will be ignored entirely.
//
void AcceptMessageFromResponseMap::ExecuteFast(ExecutionContext& context)
{
	const ResponseMap& themap = context.Scope.GetOriginalDescription().GetResponseMap(MapName);
	const std::vector<ResponseMapEntry*>& mapentries = themap.GetEntries();

	Dispatch(mapentries, context, context.Scope.TaskOrigin);
}

RValuePtr AcceptMessageFromResponseMap::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}

template <typename TraverserT>
void AcceptMessageFromResponseMap::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	
	const std::vector<ResponseMapEntry*>& mapentries = traverser.GetCurrentScope()->GetResponseMap(MapName).GetEntries();
	for(std::vector<ResponseMapEntry*>::const_iterator iter = mapentries.begin(); iter != mapentries.end(); ++iter)
		(*iter)->GetResponseBlock()->Traverse(traverser);
}

void AcceptMessageFromResponseMap::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void AcceptMessageFromResponseMap::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

//
// Retrieve the ID of the task which forked this task
//
RValuePtr GetTaskCaller::ExecuteAndStoreRValue(ExecutionContext& context)
{
	TaskHandle id = context.Scope.FindTaskOrigin();
	return RValuePtr(new TaskHandleRValue(id));
}

void GetTaskCaller::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do.
}


//
// Retrieve the ID of the task which sent the most recently accepted message
//
RValuePtr GetMessageSender::ExecuteAndStoreRValue(ExecutionContext& context)
{
	TaskHandle id = context.Scope.FindLastMessageOrigin();
	return RValuePtr(new TaskHandleRValue(id));
}

void GetMessageSender::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do.
}


namespace
{

	//
	// Wait for an incoming message from another task, and then act on it as needed
	//
	// This function blocks until a message is matched and accepted. Any messages which
	// are not matched are discarded immediately.
	//
	void Dispatch(const std::vector<ResponseMapEntry*>& mapentries, ExecutionContext& context, HandleType taskorigin)
	{
		while(true)
		{
			// We use an artificial scope here to ensure that the auto_ptr destructs on time
			{
				std::auto_ptr<Threads::MessageInfo> msginfo(Threads::WaitForEvent());
				if(!msginfo.get())
					continue;

				bool messagerecognized = false;
				Block* messageblock = NULL;

				for(std::vector<ResponseMapEntry*>::const_iterator iter = mapentries.begin(); iter != mapentries.end(); ++iter)
				{
					const ResponseMapEntry& mapentry = **iter;

					if(msginfo->MessageName != mapentry.GetMessageName())
						continue;

					bool ignore = false;

					const std::list<EpochVariableTypeID>& payloadtypes = mapentry.GetPayloadTypes();

					if(payloadtypes.size() != msginfo->PayloadTypes.size())
						continue;

					std::list<EpochVariableTypeID>::const_iterator msginfoiter = msginfo->PayloadTypes.begin();
					for(std::list<EpochVariableTypeID>::const_iterator payloaditer = payloadtypes.begin(); payloaditer != payloadtypes.end(); ++payloaditer)
					{
						if(*payloaditer != *msginfoiter)
						{
							ignore = true;
							break;
						}

						++msginfoiter;
					}

					if(ignore)
						continue;

					messagerecognized = true;
					messageblock = mapentry.GetResponseBlock();
					break;
				}

				if(!messagerecognized)
					continue;

				void* heapptr = msginfo->StorageBlock->GetStartOfStorage();
				for(std::list<EpochVariableTypeID>::const_iterator storageiter = msginfo->PayloadTypes.begin(); storageiter != msginfo->PayloadTypes.end(); ++storageiter)
				{
					switch(*storageiter)
					{
					case EpochVariableType_Integer:
						PushValueOntoStack<TypeInfo::IntegerT>(context.Stack, *reinterpret_cast<IntegerVariable::BaseStorage*>(heapptr));
						break;

					case EpochVariableType_Integer16:
						PushValueOntoStack<TypeInfo::Integer16T>(context.Stack, *reinterpret_cast<Integer16Variable::BaseStorage*>(heapptr));
						break;

					case EpochVariableType_Real:
						PushValueOntoStack<TypeInfo::RealT>(context.Stack, *reinterpret_cast<RealVariable::BaseStorage*>(heapptr));
						break;

					case EpochVariableType_Boolean:
						PushValueOntoStack<TypeInfo::BooleanT>(context.Stack, *reinterpret_cast<BooleanVariable::BaseStorage*>(heapptr));
						break;

					case EpochVariableType_String:
						PushValueOntoStack<TypeInfo::StringT>(context.Stack, *reinterpret_cast<StringVariable::BaseStorage*>(heapptr));
						break;

					case EpochVariableType_Array:
						PushValueOntoStack<TypeInfo::ArrayT>(context.Stack, *reinterpret_cast<ArrayVariable::BaseStorage*>(heapptr));
						break;

					default:
						throw NotImplementedException("Cannot process message payload of this type");
					}

					heapptr = reinterpret_cast<Byte*>(heapptr) + TypeInfo::GetStorageSize(*storageiter);
				}

				std::auto_ptr<ActivatedScope> newparamscope(new ActivatedScope(*messageblock->GetBoundScope()->ParentScope));
				newparamscope->ParentScope = &context.Scope;
				newparamscope->BindToStack(context.Stack);

				std::auto_ptr<ActivatedScope> newcodescope(new ActivatedScope(*messageblock->GetBoundScope()));
				newcodescope->ParentScope = newparamscope.get();
				newcodescope->LastMessageOrigin = msginfo->Origin;
				newcodescope->TaskOrigin = taskorigin;
				messageblock->ExecuteBlock(ExecutionContext(context.RunningProgram, *newcodescope, context.Stack, context.FlowResult), NULL);

				newparamscope->Exit(context.Stack);
				break;
			}
		}
	}

}

