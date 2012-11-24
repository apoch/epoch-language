//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Definition of the VM wrapper class
//

#include "pch.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Virtual Machine/Marshaling.h"

#include "JIT/JIT.h"

#include "Metadata/ActiveScope.h"
#include "Metadata/TypeInfo.h"

#include "Bytecode/Instructions.h"
#include "Bytecode/EntityTags.h"

#include "Utility/DLLPool.h"
#include "Utility/EraseDeadHandles.h"

#ifdef EPOCHVM_VISUAL_DEBUGGER
#include "Visual Debugger/VisualDebugger.h"
#endif

#include <limits>
#include <list>
#include <iostream>
#include <sstream>


using namespace VM;
using namespace Metadata;


#ifdef EPOCHVM_VISUAL_DEBUGGER

//
// Static variables for the virtual machine class
//
bool VirtualMachine::VisualDebuggerEnabled = false;

#endif



namespace
{

	//
	// Helper shim - invoke the given Epoch function within the given context
	//
	// Whenever an Epoch function is invoked, its handle is passed to this function, along with
	// the execution context in which the virtual machine is running the Epoch code.
	//
	void FunctionInvocationHelper(StringHandle namehandle, ExecutionContext& context)
	{
		context.Execute(context.OwnerVM.GetFunctionInstructionOffset(namehandle), context.OwnerVM.GetScopeDescription(namehandle), true);
	}

}


#define PROFILING_ENABLED
#ifdef PROFILING_ENABLED
#include "Utility/Profiling.h"
#include "User Interface/Output.h"
Profiling::Timer GlobalTimer;
#endif

//
// Construct and initialize a virtual machine
//
// Mainly useful for forking the visual debugger thread as necessary
//
VirtualMachine::VirtualMachine()
{
#ifdef EPOCHVM_VISUAL_DEBUGGER
	if(VisualDebuggerEnabled)
		VisualDebugger::ForkDebuggerThread(this);
#endif
}

//
// Enable the display and operation of the visual VM debugger
//
void VirtualMachine::EnableVisualDebugger()
{
#ifdef EPOCHVM_VISUAL_DEBUGGER
	VisualDebuggerEnabled = true;
#endif
}

//
// Initialize the bindings of standard library functions
//
void VirtualMachine::InitStandardLibraries(unsigned* testharness)
{
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochLibrary.DLL");

	typedef void (STDCALL *bindtovmptr)(FunctionInvocationTable&, EntityTable&, EntityTable&, StringPoolManager&, Bytecode::EntityTag&, EpochFunctionPtr, JIT::JITTable&);
	bindtovmptr bindtovm = Marshaling::DLLPool::GetFunction<bindtovmptr>(dllhandle, "BindToVirtualMachine");

	if(!bindtovm)
		throw FatalException("Failed to load Epoch standard library");

	void ExternalDispatch(StringHandle functionname, VM::ExecutionContext& context);

	Bytecode::EntityTag customtag = Bytecode::EntityTags::CustomEntityBaseID;
	bindtovm(GlobalFunctions, Entities, Entities, PrivateStringPool, customtag, ExternalDispatch, JITHelpers);


	typedef void (STDCALL *bindtotestharnessptr)(unsigned*);
	bindtotestharnessptr bindtotest = Marshaling::DLLPool::GetFunction<bindtotestharnessptr>(dllhandle, "LinkToTestHarness");

	if(!bindtotest)
		throw FatalException("Failed to load Epoch standard library - test harness init failure");

	bindtotest(testharness);
}


//
// Execute a block of bytecode from memory
//
ExecutionResult VirtualMachine::ExecuteByteCode(Bytecode::Instruction* buffer, size_t size)
{
	std::auto_ptr<ExecutionContext> context(new ExecutionContext(*this, buffer, size));
	context->Execute(NULL, false);
	ExecutionResult result = context->GetExecutionResult();

#ifdef _DEBUG
	if(context->State.Stack.GetAllocatedStack() > 0)
		throw FatalException("Stack leakage occurred!");
#endif

	return result;
}


//
// Store a string in the global string pool, allocating an ID as necessary
//
EPOCHVM StringHandle VirtualMachine::PoolString(const std::wstring& stringdata)
{
	return PrivateStringPool.PoolFast(stringdata);
}

//
// Store a string in the global string pool
//
void VirtualMachine::PoolString(StringHandle handle, const std::wstring& stringdata)
{
	PrivateStringPool.Pool(handle, stringdata);
}

//
// Retrieve a pooled string from the global pool
//
EPOCHVM const std::wstring& VirtualMachine::GetPooledString(StringHandle handle) const
{
	return PrivateStringPool.GetPooledString(handle);
}

//
// Given a pooled string value, retrieve the corresponding handle
//
StringHandle VirtualMachine::GetPooledStringHandle(const std::wstring& value)
{
	return PrivateStringPool.Pool(value);
}

//
// Retrieve a buffer pointer from the list of allocated buffers
//
EPOCHVM void* VirtualMachine::GetBuffer(BufferHandle handle)
{
	Threads::CriticalSection::Auto lock(BufferCritSec);

	std::map<StringHandle, std::vector<Byte> >::iterator iter = Buffers.find(handle);
	if(iter == Buffers.end())
		throw FatalException("Invalid buffer handle");

	return &iter->second[0];
}

//
// Retrive the size of the given buffer, in bytes
//
EPOCHVM size_t VirtualMachine::GetBufferSize(BufferHandle handle) const
{
	Threads::CriticalSection::Auto lock(BufferCritSec);

	std::map<StringHandle, std::vector<Byte> >::const_iterator iter = Buffers.find(handle);
	if(iter == Buffers.end())
		throw FatalException("Invalid buffer handle");

	return iter->second.size();
}

//
// Allocate a data buffer and return its handle
//
EPOCHVM BufferHandle VirtualMachine::AllocateBuffer(size_t size)
{
	Threads::CriticalSection::Auto lock(BufferCritSec);

	BufferHandle ret = BufferHandleAlloc.AllocateHandle(Buffers);
	Buffers[ret].swap(std::vector<Byte>(size, 0));
	return ret;
}

//
// Allocate a copy of an existing data buffer and return the new handle
//
BufferHandle VirtualMachine::CloneBuffer(BufferHandle handle)
{
	Threads::CriticalSection::Auto lock(BufferCritSec);

	std::map<StringHandle, std::vector<Byte> >::const_iterator iter = Buffers.find(handle);
	if(iter == Buffers.end())
		throw FatalException("Invalid buffer handle");

	BufferHandle ret = BufferHandleAlloc.AllocateHandle(Buffers);
	Buffers[ret].swap(std::vector<Byte>(iter->second.begin(), iter->second.end()));
	return ret;
}

//
// Add a function to the global namespace
//
void VirtualMachine::AddFunction(StringHandle name, EpochFunctionPtr funcptr)
{
	if(GlobalFunctions.find(name) != GlobalFunctions.end())
		throw InvalidIdentifierException("Function identifier is already in use");
	
	GlobalFunctions.insert(std::make_pair(name, funcptr));
}

//
// Add a user-implemented function to the global namespace
//
void VirtualMachine::AddFunction(StringHandle name, size_t instructionoffset)
{
	if(GlobalFunctions.find(name) != GlobalFunctions.end())
		throw InvalidIdentifierException("Function identifier is already in use");
	
	if(GlobalFunctionOffsets.find(name) != GlobalFunctionOffsets.end())
		throw FatalException("Global function code offset has already been cached, but function was not found in the global namespace");
	
	GlobalFunctions.insert(std::make_pair(name, &FunctionInvocationHelper));
	GlobalFunctionOffsets.insert(std::make_pair(name, instructionoffset));
}

//
// Invoke a function in the currently active namespace
//
void VirtualMachine::InvokeFunction(StringHandle namehandle, ExecutionContext& context)
{
	FunctionInvocationTable::const_iterator iter = GlobalFunctions.find(namehandle);
	if(iter == GlobalFunctions.end())
		throw InvalidIdentifierException("No function with that identifier was found");

	iter->second(namehandle, context);
}

//
// Retrieve the byte offset in the bytecode stream where the given function begins
//
size_t VirtualMachine::GetFunctionInstructionOffset(StringHandle functionname) const
{
	OffsetMap::const_iterator iter = GlobalFunctionOffsets.find(functionname);
	if(iter == GlobalFunctionOffsets.end())
		throw InvalidIdentifierException("No function with that identifier was found");

	return iter->second;
}

size_t VirtualMachine::GetFunctionInstructionOffsetNoThrow(StringHandle functionname) const
{
	OffsetMap::const_iterator iter = GlobalFunctionOffsets.find(functionname);
	if(iter == GlobalFunctionOffsets.end())
		return 0;

	return iter->second;
}


//
// Add metadata for a lexical scope
//
void VirtualMachine::AddLexicalScope(StringHandle name)
{
	LexicalScopeDescriptions.insert(std::make_pair(name, ScopeDescription()));
}

//
// Retrieve a lexical scope description
//
const ScopeDescription& VirtualMachine::GetScopeDescription(StringHandle name) const
{
	ScopeMap::const_iterator iter = LexicalScopeDescriptions.find(name);
	if(iter == LexicalScopeDescriptions.end())
		throw InvalidIdentifierException("No lexical scope has been attached to the given identifier");

	return iter->second;
}
//
// Retrieve a mutable lexical scope description
//
ScopeDescription& VirtualMachine::GetScopeDescription(StringHandle name)
{
	ScopeMap::iterator iter = LexicalScopeDescriptions.find(name);
	if(iter == LexicalScopeDescriptions.end())
		throw InvalidIdentifierException("No lexical scope has been attached to the given identifier");

	return iter->second;
}


//
// Initialize a virtual machine execution context
//
ExecutionContext::ExecutionContext(VirtualMachine& ownervm, Bytecode::Instruction* codebuffer, size_t codesize)
	: OwnerVM(ownervm),
	  CodeBuffer(codebuffer),
	  CodeBufferSize(codesize),
	  InstructionOffset(0),
	  Variables(NULL),
	  GarbageTick_Buffers(0),
	  GarbageTick_Strings(0),
	  GarbageTick_Structures(0)

{
	ActiveScope::InitAllocator();
	Load();
	InstructionOffset = 0;
}

//
// Shift a function onto the call stack and invoke the corresponding code
//
void ExecutionContext::Execute(size_t offset, const ScopeDescription& scope, bool returnonfunctionexit)
{
	InstructionOffsetStack.push(InstructionOffset);

	InstructionOffset = offset;
	Execute(&scope, returnonfunctionexit);
}

//
// Run available code until halted for some reason
//
// The reason will be provided in the attached execution context data,
// as well as any other contextual information, eg. exception payloads.
//
void ExecutionContext::Execute(const ScopeDescription* scope, bool returnonfunctionexit)
{
	// Automatically cleanup the stack as needed
	struct autoexit_functionscopes
	{
		autoexit_functionscopes(ExecutionContext* thisptr, ActiveScope* scopeptr)
			: ThisPtr(thisptr),
			  ScopePtr(scopeptr)
		{ }

		~autoexit_functionscopes()
		{
			if(!ScopePtr)
				return;

			ThisPtr->Variables->PopScopeOffStack(*ThisPtr);
			bool hasreturn = ThisPtr->Variables->HasReturnVariable();
			ActiveScope* parent = ThisPtr->Variables->ParentScope;
			delete ThisPtr->Variables;
			ThisPtr->Variables = parent;
			if(hasreturn)
			{
				ThisPtr->State.ReturnValueRegister.PushOntoStack(ThisPtr->State.Stack);
			}
		}

		ActiveScope* ScopePtr;
		ExecutionContext* ThisPtr;
	};

	struct autoexit
	{
		explicit autoexit(ExecutionContext* thisptr)
			: ThisPtr(thisptr)
		{
			FunctionScopes.reserve(1024);
		}

		~autoexit()
		{
			while(!FunctionScopes.empty())
				UnwindOutermostFunctionStack();
		}

		void UnwindOutermostFunctionStack()
		{
			if(!FunctionScopes.empty())
			{
				delete FunctionScopes.back();
				FunctionScopes.pop_back();
			}
		}

		void CleanUpScope(ActiveScope* scope)
		{
			FunctionScopes.push_back(new autoexit_functionscopes(ThisPtr, scope));
		}

		void MarkEmptyScope()
		{
			FunctionScopes.push_back(new autoexit_functionscopes(ThisPtr, NULL));
		}

		std::vector<autoexit_functionscopes*> FunctionScopes;

		ExecutionContext* ThisPtr;
	} onexit(this);

	// By default, assume everything is alright
	State.Result.ResultType = ExecutionResult::EXEC_RESULT_OK;

	std::stack<size_t> chainoffsets;
	std::stack<bool> chainrepeats;
	std::stack<bool> returnonfunctionexitstack;

	returnonfunctionexitstack.push(returnonfunctionexit);

	UByte* instructionbuffer = &CodeBuffer[0];

	try
	{
		// Run the given chunk of code
		while(State.Result.ResultType == ExecutionResult::EXEC_RESULT_OK && InstructionOffset < CodeBufferSize)
		{
			switch(instructionbuffer[InstructionOffset++])
			{
			case Bytecode::Instructions::Halt:		// Halt the machine
				State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;

#ifdef PROFILING_ENABLED
				{
					Integer64 accumulatedtime = GlobalTimer.GetAccumulatedMs();
					UI::OutputStream output;
					output << L"VM profiler: " << accumulatedtime << std::endl;
				}
#endif
				break;

			case Bytecode::Instructions::NoOp:		// Do nothing for a cycle
				break;

			case Bytecode::Instructions::Return:	// Return execution control to the parent context
				State.Result.ResultType = ExecutionResult::EXEC_RESULT_RETURN;
				break;

			case Bytecode::Instructions::SetRetVal:	// Set return value register
				{
					size_t variableindex = Fetch<size_t>();
					Variables->CopyToRegister(variableindex, State.ReturnValueRegister);
					continue;
				}
				break;

			case Bytecode::Instructions::CopyFromStructure:
				{
					StringHandle variablename = Fetch<StringHandle>();
					StringHandle membername = Fetch<StringHandle>();
					
					StructureHandle readstruct = Variables->Read<StructureHandle>(variablename);

					ActiveStructure& structure = OwnerVM.GetStructure(readstruct);

					size_t memberindex = structure.Definition.FindMember(membername);
					EpochTypeID membertype = structure.Definition.GetMemberType(memberindex);

					if(GetTypeFamily(membertype) == EpochTypeFamily_SumType)
					{
						membertype = structure.ReadSumTypeMemberType(memberindex);
						State.ReturnValueRegister.SumType = true;
					}
					else
					{
						State.ReturnValueRegister.SumType = false;
					}

					switch(membertype)
					{
					case EpochType_Integer:
						State.ReturnValueRegister.Set(structure.ReadMember<Integer32>(memberindex));
						break;

					case EpochType_Integer16:
						State.ReturnValueRegister.Set(structure.ReadMember<Integer16>(memberindex));
						break;

					case EpochType_Boolean:
						State.ReturnValueRegister.Set(structure.ReadMember<bool>(memberindex));
						break;

					case EpochType_Buffer:
						State.ReturnValueRegister.SetBuffer(structure.ReadMember<Integer32>(memberindex));
						break;

					case EpochType_Real:
						State.ReturnValueRegister.Set(structure.ReadMember<Real32>(memberindex));
						break;

					case EpochType_String:
						State.ReturnValueRegister.SetString(structure.ReadMember<StringHandle>(memberindex));
						break;

					case EpochType_Function:
						State.ReturnValueRegister.SetFunction(structure.ReadMember<StringHandle>(memberindex));
						break;

					case EpochType_Nothing:
						State.ReturnValueRegister.Type = EpochType_Nothing;
						break;

					default:
						if(GetTypeFamily(membertype) == EpochTypeFamily_Structure || GetTypeFamily(membertype) == EpochTypeFamily_TemplateInstance)
							State.ReturnValueRegister.SetStructure(structure.ReadMember<StructureHandle>(memberindex), membertype);
						else
							throw FatalException("Unhandled structure member type");

						break;
					}

					continue;
				}
				break;

			case Bytecode::Instructions::CopyToStructure:
				{
					StringHandle variablename = Fetch<StringHandle>();
					StringHandle actualmember = Fetch<StringHandle>();
					
					StructureHandle readstruct = Variables->Read<StructureHandle>(variablename);

					ActiveStructure& structure = OwnerVM.GetStructure(readstruct);

					size_t memberindex = structure.Definition.FindMember(actualmember);
					EpochTypeID membertype = structure.Definition.GetMemberType(memberindex);
					WriteStructureMember(structure, memberindex, membertype);

					continue;
				}
				break;

			case Bytecode::Instructions::Push:		// Push something onto the stack
				{
					EpochTypeID pushedtype = Fetch<EpochTypeID>();
					switch(pushedtype)
					{
					case EpochType_Integer:
						{
							Integer32 value = Fetch<Integer32>();
							State.Stack.PushValue(value);
						}
						break;

					case EpochType_Integer16:
						{
							Integer16 value = Fetch<Integer16>();
							State.Stack.PushValue(value);
						}
						break;

					case EpochType_String:
						{
							StringHandle handle = Fetch<StringHandle>();
							State.Stack.PushValue(handle);
						}
						break;

					case EpochType_Boolean:
						{
							bool value = Fetch<bool>();
							State.Stack.PushValue(value);
						}
						break;

					case EpochType_Real:
						{
							Real32 value = Fetch<Real32>();
							State.Stack.PushValue(value);
						}
						break;

					case EpochType_Buffer:
						{
							BufferHandle handle = Fetch<BufferHandle>();
							State.Stack.PushValue(handle);
						}
						break;

					default:
						throw NotImplementedException("Cannot execute PUSH instruction: unsupported type");
					}

					continue;
				}
				break;

			case Bytecode::Instructions::BindRef:
				{
					size_t frames = Fetch<size_t>();
					size_t targetindex = Fetch<size_t>();

					ActiveScope* vars = Variables;
					if(frames == 0xffffffff)
					{
						while(vars->ParentScope)
							vars = vars->ParentScope;
					}
					/*else
					{
						while(frames > 0)
						{
							vars = vars->ParentScope;
							--frames;
						}
					}*/

					if(vars->GetOriginalDescription().IsReference(targetindex))
					{
						State.Stack.PushValue(vars->GetReferenceType(targetindex));
						State.Stack.PushValue(vars->GetReferenceTarget(targetindex));
					}
					else
					{
						State.Stack.PushValue(vars->GetActualType(targetindex));
						State.Stack.PushValue(vars->GetVariableStorageLocationByIndex(targetindex));
					}

					continue;
				}
				break;

			case Bytecode::Instructions::BindMemberRef:
				{
					StringHandle member = Fetch<StringHandle>();

					void* storagelocation = State.Stack.PopValue<void*>();
					State.Stack.PopValue<EpochTypeID>();

					StructureHandle* phandle = reinterpret_cast<StructureHandle*>(storagelocation);
					StructureHandle handle = *phandle;

					ActiveStructure& structure = OwnerVM.GetStructure(handle);
					const StructureDefinition& definition = structure.Definition;

					size_t memberindex = definition.FindMember(member);
					size_t offset = definition.GetMemberOffset(memberindex);

					EpochTypeID membertype = definition.GetMemberType(memberindex);
					if(GetTypeFamily(membertype) == EpochTypeFamily_SumType)
					{
						void* typeptr = &(structure.Storage[0]) + offset;
						membertype = *reinterpret_cast<EpochTypeID*>(typeptr);
						offset += sizeof(EpochTypeID);
					}

					void* memberstoragelocation = &(structure.Storage[0]) + offset;

					State.Stack.PushValue(membertype);
					State.Stack.PushValue(memberstoragelocation);

					continue;
				}
				break;

			case Bytecode::Instructions::BindMemberByHandle:
				{
					StringHandle member = Fetch<StringHandle>();
					StructureHandle handle = State.Stack.PopValue<StructureHandle>();

					ActiveStructure& structure = OwnerVM.GetStructure(handle);
					const StructureDefinition& definition = structure.Definition;
					size_t memberindex = definition.FindMember(member);
					size_t offset = definition.GetMemberOffset(memberindex);

					EpochTypeID membertype = definition.GetMemberType(memberindex);
					if(GetTypeFamily(membertype) == EpochTypeFamily_SumType)
					{
						void* typeptr = &(structure.Storage[0]) + offset;
						membertype = *reinterpret_cast<EpochTypeID*>(typeptr);
						offset += sizeof(EpochTypeID);
					}

					void* memberstoragelocation = &(structure.Storage[0]) + offset;

					State.Stack.PushValue(membertype);
					State.Stack.PushValue(memberstoragelocation);

					continue;
				}
				break;

			case Bytecode::Instructions::Pop:		// Pop some stuff off the stack
				{
					EpochTypeID poppedtype = Fetch<EpochTypeID>();
					State.Stack.Pop(GetStorageSize(poppedtype));
					continue;
				}
				break;

			case Bytecode::Instructions::Read:		// Read a variable's value and place it on the stack
				{
					StringHandle variablename = Fetch<StringHandle>();
					Variables->PushOntoStack(variablename, State.Stack);
					continue;
				}
				break;

			case Bytecode::Instructions::ReadStack:
				{
					ActiveScope* vars = Variables;
					size_t frames = Fetch<size_t>();

					if(frames == 0xffffffff)
					{
						while(vars->ParentScope)
							vars = vars->ParentScope;
					}
					/*else
					{
						while(frames > 0)
						{
							vars = vars->ParentScope;
							--frames;
						}
					}*/
					
					char* stackptr = reinterpret_cast<char*>(vars->GetStartOfLocals());
					stackptr -= Fetch<size_t>();
					size_t size = Fetch<size_t>();
					State.Stack.Push(size);
					memmove(State.Stack.GetCurrentTopOfStack(), stackptr - size, size);
					continue;
				}
				break;

			case Bytecode::Instructions::ReadParam:
				{
					ActiveScope* vars = Variables;
					size_t frames = Fetch<size_t>();

					if(frames == 0xffffffff)
					{
						while(vars->ParentScope)
							vars = vars->ParentScope;
					}
					/*else
					{
						while(frames > 0)
						{
							vars = vars->ParentScope;
							--frames;
						}
					}*/

					char* stackptr = reinterpret_cast<char*>(vars->GetStartOfParams());
					stackptr += Fetch<size_t>();
					size_t size = Fetch<size_t>();
					State.Stack.Push(size);
					memmove(State.Stack.GetCurrentTopOfStack(), stackptr, size);
					continue;
				}
				break;

			case Bytecode::Instructions::ReadRef:	// Read a reference's target value and place it on the stack
				{
					void* targetstorage = State.Stack.PopValue<void*>();
					EpochTypeID targettype = State.Stack.PopValue<EpochTypeID>();
					Variables->PushOntoStack(targetstorage, targettype, State.Stack);
					continue;
				}
				break;

			case Bytecode::Instructions::ReadRefAnnotated:
				{
					void* targetstorage = State.Stack.PopValue<void*>();
					EpochTypeID targettype = State.Stack.PopValue<EpochTypeID>();
					Variables->PushOntoStack(targetstorage, targettype, State.Stack);
					State.Stack.PushValue(targettype);
					continue;
				}
				break;

			case Bytecode::Instructions::Assign:	// Write a value to a variable's position on the stack
				{
					void* targetstorage = State.Stack.PopValue<void*>();
					EpochTypeID targettype = State.Stack.PopValue<EpochTypeID>();
					Variables->WriteFromStack(targetstorage, targettype, State.Stack);
					continue;
				}
				break;

			case Bytecode::Instructions::AssignThroughIdentifier:
				{
					EpochTypeID targettype = State.Stack.PopValue<EpochTypeID>();
					StringHandle identifier = State.Stack.PopValue<StringHandle>();
					Variables->WriteFromStack(Variables->GetVariableStorageLocation(identifier), targettype, State.Stack);
					continue;
				}
				break;

			case Bytecode::Instructions::AssignSumType:
				{
					void* targetstorage = State.Stack.PopValue<void*>();
					State.Stack.PopValue<EpochTypeID>();
					EpochTypeID actualtype = State.Stack.PopValue<EpochTypeID>();
					Variables->WriteFromStack(targetstorage, actualtype, State.Stack);
					void* typestorage = reinterpret_cast<char*>(targetstorage) - sizeof(EpochTypeID);
					*reinterpret_cast<EpochTypeID*>(typestorage) = actualtype;
					continue;
				}
				break;

			case Bytecode::Instructions::Invoke:	// Invoke a built-in or user-defined function
				{
					StringHandle functionname = Fetch<StringHandle>();
					InvokedFunctionStack.push_back(functionname);

					OwnerVM.InvokeFunction(functionname, *this);
					InvokedFunctionStack.pop_back();

					if(State.Result.ResultType == ExecutionResult::EXEC_RESULT_HALT)
						throw FatalException("Unexpected VM halt");
				}
				break;

			case Bytecode::Instructions::InvokeOffset:
				{
					StringHandle functionname = Fetch<StringHandle>();
					InvokedFunctionStack.push_back(functionname);
					size_t internaloffset = Fetch<size_t>();

					InstructionOffsetStack.push(InstructionOffset);

					InstructionOffset = internaloffset;
					scope = &OwnerVM.GetScopeDescription(functionname);

					returnonfunctionexitstack.push(false);
				}
				break;

			case Bytecode::Instructions::InvokeIndirect:
				{
					StringHandle varname = Fetch<StringHandle>();
					StringHandle functionname = Variables->Read<StringHandle>(varname);

					InvokedFunctionStack.push_back(functionname);
					OwnerVM.InvokeFunction(functionname, *this);
					InvokedFunctionStack.pop_back();
				}
				break;

			case Bytecode::Instructions::InvokeNative:
				{
					StringHandle target = Fetch<StringHandle>();
					OwnerVM.JITExecs.find(target)->second(State.Stack.GetMutableStackPtr(), this);
				}
				break;

			case Bytecode::Instructions::BeginEntity:
				{
					size_t originaloffset = InstructionOffset - 1;
					Bytecode::EntityTag tag = static_cast<Bytecode::EntityTag>(Fetch<Integer32>());
					StringHandle name = Fetch<StringHandle>();

					if(tag == Bytecode::EntityTags::Function)
					{
						if(scope->GetVariableCount())
						{
							Variables = new ActiveScope(*scope, Variables);
							Variables->BindParametersToStack(*this);
							Variables->PushLocalsOntoStack(*this);
							onexit.CleanUpScope(Variables);
						}
						else
							onexit.MarkEmptyScope();
					}
					else if(tag == Bytecode::EntityTags::FreeBlock)
					{
						/*scope = &OwnerVM.GetScopeDescription(name);
						if(scope->GetVariableCount())
						{
							Variables = new ActiveScope(*scope, Variables);
							Variables->BindParametersToStack(*this);
							Variables->PushLocalsOntoStack(*this);
							onexit.CleanUpScope(Variables);
						}
						else
							onexit.MarkEmptyScope();
						*/
					}
					else if(tag == Bytecode::EntityTags::Globals)
					{
						scope = &OwnerVM.GetScopeDescription(name);
						if(scope->GetVariableCount())
						{
							Variables = new ActiveScope(*scope, Variables);
							Variables->BindParametersToStack(*this);
							Variables->PushLocalsOntoStack(*this);
							onexit.CleanUpScope(Variables);
						}
						else
							onexit.MarkEmptyScope();
					}
					else if(tag == Bytecode::EntityTags::PatternMatchingResolver || tag == Bytecode::EntityTags::TypeResolver)
					{
						// Do nothing
					}
					else
					{
						EntityReturnCode code = OwnerVM.GetEntityMetaControl(tag)(*this);
						if(code == ENTITYRET_EXECUTE_CURRENT_LINK_IN_CHAIN)
						{
							/*scope = &OwnerVM.GetScopeDescription(name);
							if(scope->GetVariableCount())
							{
								Variables = new ActiveScope(*scope, Variables);
								Variables->BindParametersToStack(*this);
								Variables->PushLocalsOntoStack(*this);
								onexit.CleanUpScope(Variables);
							}
							else
								onexit.MarkEmptyScope();
							*/
						}
						else if(code == ENTITYRET_PASS_TO_NEXT_LINK_IN_CHAIN)
							InstructionOffset = OwnerVM.GetEntityEndOffset(originaloffset) + 1;
						else if(code == ENTITYRET_EXIT_CHAIN)
							InstructionOffset = OwnerVM.GetChainEndOffset(chainoffsets.top());
						else if(code == ENTITYRET_EXECUTE_AND_REPEAT_ENTIRE_CHAIN)
						{
							chainrepeats.top() = true;
							/*scope = &OwnerVM.GetScopeDescription(name);
							if(scope->GetVariableCount())
							{
								Variables = new ActiveScope(*scope, Variables);
								Variables->BindParametersToStack(*this);
								Variables->PushLocalsOntoStack(*this);
								onexit.CleanUpScope(Variables);
							}
							else
								onexit.MarkEmptyScope();
							*/
						}
						else
							throw FatalException("Invalid return code from entity meta-control");
					}

					continue;
				}
				break;

			case Bytecode::Instructions::EndEntity:
				/*onexit.CleanUpTopmostScope();
				if(Variables)
					scope = &Variables->GetOriginalDescription();
				else
					scope = NULL;
				*/
				if(!chainoffsets.empty())
				{
					if(chainrepeats.top())
						InstructionOffset = chainoffsets.top() + 1;
					else
						InstructionOffset = OwnerVM.GetChainEndOffset(chainoffsets.top());
				}
				CollectGarbage();
				continue;
				break;

				
			case Bytecode::Instructions::BeginChain:
				chainoffsets.push(InstructionOffset - 1);
				chainrepeats.push(false);
				continue;
				break;

			case Bytecode::Instructions::EndChain:
				chainoffsets.pop();
				chainrepeats.pop();
				continue;
				break;

			case Bytecode::Instructions::InvokeMeta:
				{
					Bytecode::EntityTag tag = static_cast<Bytecode::EntityTag>(Fetch<Integer32>());
					EntityReturnCode code = OwnerVM.GetEntityMetaControl(tag)(*this);

					switch(code)
					{
					case ENTITYRET_EXIT_CHAIN:
						InstructionOffset = OwnerVM.GetChainEndOffset(chainoffsets.top());
						break;

					case ENTITYRET_PASS_TO_NEXT_LINK_IN_CHAIN:
					case ENTITYRET_EXECUTE_CURRENT_LINK_IN_CHAIN:
						break;
					
					case ENTITYRET_EXECUTE_AND_REPEAT_ENTIRE_CHAIN:
						chainrepeats.top() = true;
						break;

					default:
						throw FatalException("Invalid return code from entity meta-control");
					}
				}
				continue;
				break;


			case Bytecode::Instructions::PoolString:
				Fetch<StringHandle>();
				Fetch<std::wstring>();
				continue;
				break;

			case Bytecode::Instructions::DefineLexicalScope:
				{
					Fetch<StringHandle>();
					Fetch<StringHandle>();
					Integer32 numentries = Fetch<Integer32>();
					for(Integer32 i = 0; i < numentries; ++i)
					{
						Fetch<StringHandle>();
						Fetch<EpochTypeID>();
						Fetch<VariableOrigin>();
						Fetch<bool>();
					}
				}
				continue;
				break;

			case Bytecode::Instructions::DefineStructure:
				{
					Fetch<EpochTypeID>();
					size_t numentries = Fetch<size_t>();

					for(size_t i = 0; i < numentries; ++i)
					{
						Fetch<StringHandle>();
						Fetch<EpochTypeID>();
					}
				}
				continue;
				break;

			case Bytecode::Instructions::PatternMatch:
				{
					char* stackptr = *State.Stack.GetMutableStackPtr();
					char* destptr = stackptr;
					bool matchedpattern = true;
					StringHandle originalfunction = Fetch<StringHandle>();
					size_t internaloffset = Fetch<size_t>();
					size_t paramcount = Fetch<size_t>();
					size_t instroffset = InstructionOffset;
					for(size_t i = 0; i < paramcount; ++i)
					{
						EpochTypeID paramtype = Fetch<EpochTypeID>();
						bool needsmatch = (Fetch<Byte>() != 0);
						if(needsmatch)
						{
							switch(paramtype)
							{
							case EpochType_Integer:
								{
									Integer32 valuetomatch = Fetch<Integer32>();
									Integer32 passedvalue = *reinterpret_cast<const Integer32*>(stackptr);
									if(valuetomatch != passedvalue)
										matchedpattern = false;
								}
								break;

							default:
								throw NotImplementedException("Support for pattern matching this function parameter type is not implemented");
							}
						}
						
						if(!matchedpattern)
							break;

						stackptr += GetStorageSize(paramtype);
					}

					if(matchedpattern)
					{
						stackptr = destptr;
						InstructionOffset = instroffset;
						for(size_t i = 0; i < paramcount; ++i)
						{
							EpochTypeID paramtype = Fetch<EpochTypeID>();
							bool needsmatch = (Fetch<Byte>() != 0);
							if(needsmatch)
							{
								if(destptr != stackptr)
									memmove(destptr, stackptr, GetStorageSize(paramtype));
							}
							else
								destptr += GetStorageSize(paramtype);

							stackptr += GetStorageSize(paramtype);
						}

						State.Stack.Pop(stackptr - destptr);

						InstructionOffset = internaloffset;
						scope = &OwnerVM.GetScopeDescription(originalfunction);
					}
				}
				break;

			case Bytecode::Instructions::AllocStructure:
				{
					EpochTypeID structuredescription = Fetch<EpochTypeID>();
					State.Stack.PushValue(OwnerVM.AllocateStructure(OwnerVM.GetStructureDefinition(structuredescription)));
					TickStructureGarbageCollector();
					continue;
				}
				break;

			case Bytecode::Instructions::CopyBuffer:
				{
					BufferHandle originalbuffer = State.Stack.PopValue<BufferHandle>();
					BufferHandle copiedbuffer = OwnerVM.CloneBuffer(originalbuffer);
					State.Stack.PushValue(copiedbuffer);
					TickBufferGarbageCollector();
					continue;
				}
				break;

			case Bytecode::Instructions::CopyStructure:
				{
					StructureHandle originalstructure = State.Stack.PopValue<StructureHandle>();
					StructureHandle copiedstructure = OwnerVM.DeepCopy(originalstructure);
					State.Stack.PushValue(copiedstructure);
					TickStructureGarbageCollector();
					continue;
				}
				break;

			case Bytecode::Instructions::Tag:
				{
					Fetch<StringHandle>();		// Fetch entity to which tag is attached
					size_t tagdatacount = Fetch<size_t>();
					Fetch<std::wstring>();		// Fetch tag itself
					for(size_t i = 0; i < tagdatacount; ++i)
						Fetch<std::wstring>();	// Fetch tag data

					continue;
				}
				break;

			case Bytecode::Instructions::SumTypeDef:
				{
					Fetch<EpochTypeID>();
					size_t numentries = Fetch<size_t>();

					for(size_t i = 0; i < numentries; ++i)
						Fetch<EpochTypeID>();
				}
				break;

			case Bytecode::Instructions::TypeMatch:
				{
					StringHandle dispatchfunction = Fetch<StringHandle>();
					size_t internaloffset = Fetch<size_t>();
					size_t paramcount = Fetch<size_t>();

					const char* stackptr = reinterpret_cast<const char*>(State.Stack.GetCurrentTopOfStack());

					struct paraminfo
					{
						paraminfo()
							: expectedtype(EpochType_Error),
							  flags(0)
						{ }

						EpochTypeID expectedtype;
						unsigned flags;
					};

					std::vector<paraminfo> info(paramcount, paraminfo());
					const unsigned FLAG_PROVIDED_REF = 0x01;
					const unsigned FLAG_PROVIDED_NOTHING = 0x02;
					const unsigned FLAG_INLINE_REF = 0x04;
					const unsigned FLAG_PROVIDED_NOTHING_REF = 0x08;
					const unsigned FLAG_EXPECTED_REF = 0x10;

					for(size_t i = 0; i < paramcount; ++i)
					{
						bool isref = Fetch<bool>();
						info[i].expectedtype = Fetch<EpochTypeID>();
						if(isref)
							info[i].flags |= FLAG_EXPECTED_REF;
					}

					bool match = true;
					for(size_t i = 0; i < paramcount; ++i)
					{
						EpochTypeID providedtype = Metadata::EpochType_Error;
						bool providedref = false;

						if((info[i].flags & FLAG_EXPECTED_REF) == 0)
						{
							providedtype = *reinterpret_cast<const EpochTypeID*>(stackptr);
							while(GetTypeFamily(providedtype) == EpochTypeFamily_SumType)
							{
								stackptr += sizeof(EpochTypeID);
								providedtype = *reinterpret_cast<const EpochTypeID*>(stackptr);
							}

							if(providedtype == EpochType_RefFlag)
							{
								providedref = true;
								stackptr += sizeof(EpochTypeID);

								if(info[i].expectedtype == EpochType_Nothing && *reinterpret_cast<const EpochTypeID*>(stackptr) == EpochType_Nothing)
								{
									providedtype = EpochType_Nothing;
									stackptr += sizeof(EpochTypeID);
								}
								else
								{
									const void* reftarget = *reinterpret_cast<const void* const*>(stackptr);
									stackptr += sizeof(void*);
									EpochTypeID reftype = *reinterpret_cast<const EpochTypeID*>(stackptr);
									stackptr += sizeof(EpochTypeID);
									if(reftype == EpochType_Nothing && info[i].expectedtype == EpochType_Nothing)
									{
										providedtype = reftype;
										info[i].flags |= FLAG_PROVIDED_NOTHING_REF;
									}
									else if(GetTypeFamily(reftype) == EpochTypeFamily_SumType)
									{
										const UByte* reftypeptr = reinterpret_cast<const UByte*>(reftarget) - sizeof(EpochTypeID);
										reftype = *reinterpret_cast<const EpochTypeID*>(reftypeptr);
										if(reftype == EpochType_Nothing && info[i].expectedtype == EpochType_Nothing)
											providedtype = reftype;
										else
										{
											match = false;
											break;
										}
									}
									else
									{
										match = false;
										break;
									}
								}
							}
							else if(info[i].expectedtype == EpochType_Nothing)
								stackptr += sizeof(EpochTypeID);
							else
							{
								stackptr += sizeof(EpochTypeID);
								stackptr += GetStorageSize(providedtype);
							}
						}
						else
						{
							EpochTypeID magic = *reinterpret_cast<const EpochTypeID*>(stackptr);
							if(magic == EpochType_RefFlag)
							{
								providedref = true;
								stackptr += sizeof(EpochTypeID);
								const char* reftarget = *reinterpret_cast<const char* const*>(stackptr);
								stackptr += sizeof(void*);
								providedtype = *reinterpret_cast<const EpochTypeID*>(stackptr);

								if(providedtype == EpochType_Nothing)
									info[i].flags |= FLAG_PROVIDED_NOTHING_REF;

								if(stackptr == reftarget)
								{
									stackptr += GetStorageSize(providedtype);
									info[i].flags |= FLAG_INLINE_REF;
								}

								stackptr += sizeof(EpochTypeID);

								if(GetTypeFamily(providedtype) == EpochTypeFamily_SumType)
								{
									reftarget -= sizeof(EpochTypeID);
									providedtype = *reinterpret_cast<const EpochTypeID*>(reftarget);
								}
							}
							else
							{
								match = false;
								break;
							}
						}

						if(providedtype != info[i].expectedtype && GetTypeFamily(info[i].expectedtype) != EpochTypeFamily_SumType)
						{
							match = false;
							break;
						}

						if(providedref)
							info[i].flags |= FLAG_PROVIDED_REF;
						
						if(providedtype == EpochType_Nothing)
							info[i].flags |= FLAG_PROVIDED_NOTHING;
					}

					if(match)
					{
						//
						// Adjust the stack contents to copy out the type identifiers
						// that were passed in to the type matching function, so that
						// the dispatch target gets parameters in the expected format
						// as if it were called directly.
						//
						// To accomplish this, we need to start at the final argument
						// and work our way back up to the top of the stack. Luckily,
						// we already know the offset of that argument, since we just
						// got done walking *down* to find it.
						//

						char* destptr = const_cast<char*>(stackptr);

						for(size_t i = paramcount; i-- > 0; )
						{
							EpochTypeID paramtype = info[i].expectedtype;
							bool isref = info[i].flags & FLAG_PROVIDED_REF;

							if(!isref)
							{
								size_t paramsize = GetStorageSize(paramtype);
	
								// Adjust upwards to find the offset of the argument's
								// first byte on the stack. This assumes we start with
								// stackptr pointing just past the end of the argument
								// on the stack
								stackptr -= paramsize;

								// Adjust copy destination upwards accordingly
								destptr -= paramsize;

								// Copy the value of the argument down the stack
								memmove(destptr, stackptr, paramsize);

								// Adjust the copy source to reflect the fact that we
								// need to skip past the type annotation on the stack
								stackptr -= sizeof(EpochTypeID);
							}
							else
							{
								const size_t REFERENCE_SIZE = sizeof(void*) + sizeof(EpochTypeID);

								if(info[i].flags & FLAG_INLINE_REF)
									stackptr -= sizeof(EpochTypeID);

								if(info[i].flags & FLAG_PROVIDED_NOTHING)
								{
									stackptr -= sizeof(EpochTypeID);

									if(info[i].flags & FLAG_PROVIDED_NOTHING_REF)
										stackptr -= sizeof(EpochTypeID);
								}
								else
								{
									destptr -= REFERENCE_SIZE;
									stackptr -= REFERENCE_SIZE;

									if(info[i].flags & FLAG_INLINE_REF)
									{
										const EpochTypeID* typeptr = reinterpret_cast<const EpochTypeID*>(stackptr + sizeof(void*));
										EpochTypeID actualtype = *typeptr;
										size_t storagesize = GetStorageSize(actualtype);
										*(void**)(stackptr) = (void*)(destptr + storagesize + sizeof(EpochTypeID));
										memmove(destptr, stackptr, destptr - stackptr + sizeof(EpochTypeID));
									}
									else
										memmove(destptr, stackptr, REFERENCE_SIZE);
								}

								stackptr -= sizeof(EpochTypeID);
							}
						}

						State.Stack.Pop(destptr - stackptr);

						InstructionOffset = internaloffset;
						scope = &OwnerVM.GetScopeDescription(dispatchfunction);
					}
				}
				break;

			case Bytecode::Instructions::ConstructSumType:
				{
					EpochTypeID vartype = State.Stack.PopValue<EpochTypeID>();
					size_t varsize = GetStorageSize(vartype);

					const UByte* stackptr = reinterpret_cast<const UByte*>(State.Stack.GetCurrentTopOfStack());
					const UByte* targetidptr = stackptr + varsize;

					StringHandle targetid = *reinterpret_cast<const StringHandle*>(targetidptr);

					UByte* varptr = reinterpret_cast<UByte*>(Variables->GetVariableStorageLocation(targetid));
					UByte* typeptr = varptr - sizeof(EpochTypeID);
					memmove(varptr, stackptr, varsize);

					*reinterpret_cast<EpochTypeID*>(typeptr) = vartype;

					State.Stack.Pop(varsize + sizeof(StringHandle));

					Variables->SetActualType(targetid, vartype);
				}
				break;

			case Bytecode::Instructions::TempReferenceFromRegister:
				if(State.ReturnValueRegister.Type != EpochType_Nothing)
				{
					void* stackptr = State.Stack.GetCurrentTopOfStack();
					if(!State.ReturnValueRegister.SumType)
						State.Stack.PushValue(State.ReturnValueRegister.Type);

					State.Stack.PushValue(stackptr);
				}
				break;
			
			default:
				throw FatalException("Invalid bytecode operation during execution");
			}

			if(State.Result.ResultType == ExecutionResult::EXEC_RESULT_RETURN)
			{
				State.Result.ResultType = ExecutionResult::EXEC_RESULT_OK;

				CollectGarbage();
				onexit.UnwindOutermostFunctionStack();
				if(Variables)
					scope = &Variables->GetOriginalDescription();
				else
					scope = NULL;
				InstructionOffset = InstructionOffsetStack.top();
				InstructionOffsetStack.pop();
				if(returnonfunctionexitstack.top())
					return;
				InvokedFunctionStack.pop_back();
				returnonfunctionexitstack.pop();
			}
		}
	}
	catch(const std::exception& e)
	{
		::MessageBoxA(0, e.what(), "Epoch VM Exception", MB_ICONSTOP);
		State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
	}
}


//
// Pre-process the bytecode stream and load certain bits of metadata needed for execution
//
// At the moment this primarily deals with loading functions from the bytecode stream and
// caching their instruction offsets so we can invoke them on demand during execution. We
// also need to record the set of all statically referenced string handles, so that there
// is no requirement to reparse the code itself when doing garbage collection.
//
void ExecutionContext::Load()
{
	std::stack<Bytecode::EntityTag> entitytypes;
	std::stack<size_t> entitybeginoffsets;
	std::stack<size_t> chainbeginoffsets;

	std::vector<StringHandle> jitworklist;
	std::map<StringHandle, size_t> entityoffsetmap;

	std::map<size_t, StringHandle> offsetfixups;

	InstructionOffset = 0;
	while(InstructionOffset < CodeBufferSize)
	{
		Bytecode::Instruction instruction = CodeBuffer[InstructionOffset++];
		switch(instruction)
		{
		// Operations we care about
		case Bytecode::Instructions::BeginEntity:
			{
				size_t originaloffset = InstructionOffset - 1;
				entitytypes.push(Fetch<Integer32>());
				StringHandle name = Fetch<StringHandle>();
				if(
					  entitytypes.top() == Bytecode::EntityTags::Function
				   || entitytypes.top() == Bytecode::EntityTags::PatternMatchingResolver
				   || entitytypes.top() == Bytecode::EntityTags::TypeResolver
				  )
				{
					OwnerVM.AddFunction(name, originaloffset);
				}
				entitybeginoffsets.push(originaloffset);
				entityoffsetmap[name] = originaloffset;
			}
			break;

		case Bytecode::Instructions::EndEntity:
			entitytypes.pop();
			OwnerVM.MapEntityBeginEndOffsets(entitybeginoffsets.top(), InstructionOffset - 1);
			entitybeginoffsets.pop();
			break;

		case Bytecode::Instructions::BeginChain:
			{
				size_t originaloffset = InstructionOffset - 1;
				chainbeginoffsets.push(originaloffset);
			}
			break;

		case Bytecode::Instructions::EndChain:
			OwnerVM.MapChainBeginEndOffsets(chainbeginoffsets.top(), InstructionOffset - 1);
			chainbeginoffsets.pop();
			break;

		case Bytecode::Instructions::PoolString:
			{
				StringHandle handle = Fetch<StringHandle>();
				std::wstring strvalue = Fetch<std::wstring>();
				OwnerVM.PoolString(handle, strvalue);
			}
			break;

		case Bytecode::Instructions::DefineLexicalScope:
			{
				StringHandle scopename = Fetch<StringHandle>();
				StringHandle parentscopename = Fetch<StringHandle>();
				Integer32 numentries = Fetch<Integer32>();

				OwnerVM.AddLexicalScope(scopename);
				ScopeDescription& scope = OwnerVM.GetScopeDescription(scopename);
				if(parentscopename)
					scope.ParentScope = &OwnerVM.GetScopeDescription(parentscopename);

				for(Integer32 i = 0; i < numentries; ++i)
				{
					StringHandle entryname = Fetch<StringHandle>();
					EpochTypeID type = Fetch<EpochTypeID>();
					VariableOrigin origin = Fetch<VariableOrigin>();
					bool isreference = Fetch<bool>();

					scope.AddVariable(OwnerVM.GetPooledString(entryname), entryname, 0, type, isreference, origin);
				}
			}
			break;

		case Bytecode::Instructions::DefineStructure:
			{
				EpochTypeID structuretypeid = Fetch<EpochTypeID>();
				size_t numentries = Fetch<size_t>();

				for(size_t i = 0; i < numentries; ++i)
				{
					StringHandle identifier = Fetch<StringHandle>();
					EpochTypeID type = Fetch<EpochTypeID>();
					const StructureDefinition* structdefinition = NULL;
					const VariantDefinition* variantdefinition = NULL;
					if(GetTypeFamily(type) == EpochTypeFamily_Structure || GetTypeFamily(type) == EpochTypeFamily_TemplateInstance)
						structdefinition = &OwnerVM.GetStructureDefinition(type);
					else if(GetTypeFamily(type) == EpochTypeFamily_SumType)
						variantdefinition = &OwnerVM.VariantDefinitions.find(type)->second;
					OwnerVM.StructureDefinitions[structuretypeid].AddMember(identifier, type, structdefinition, variantdefinition);
				}
			}
			break;

		case Bytecode::Instructions::SumTypeDef:
			{
				EpochTypeID sumtypeid = Fetch<EpochTypeID>();
				size_t numentries = Fetch<size_t>();

				for(size_t i = 0; i < numentries; ++i)
				{
					EpochTypeID basetype = Fetch<EpochTypeID>();
					size_t basetypesize = GetStorageSize(basetype);
					OwnerVM.VariantDefinitions[sumtypeid].AddBaseType(basetype, basetypesize);
				}
			}
			break;

		case Bytecode::Instructions::Tag:
			{
				StringHandle entity = Fetch<StringHandle>();
				size_t tagdatacount = Fetch<size_t>();
				std::vector<std::wstring> metadata;
				std::wstring tag = Fetch<std::wstring>();
				for(size_t i = 0; i < tagdatacount; ++i)
					metadata.push_back(Fetch<std::wstring>());

				if(tag == L"external")
				{
					if(tagdatacount != 2)
						throw FatalException("Incorrect number of metadata tag parameters for external marshaled function");

					RegisterMarshaledExternalFunction(entity, metadata[0], metadata[1]);
				}
				else if(tag == L"native")
				{
					jitworklist.push_back(entity);
					OwnerVM.JITExecs[entity] = 0;		// Flag the entity so subsequent bytecode will be converted to InvokeNative on this func
				}
				else
					throw FatalException("Unrecognized entity meta-tag in bytecode");
			}
			break;


		// Single-byte operations with no payload
		case Bytecode::Instructions::Halt:
		case Bytecode::Instructions::NoOp:
		case Bytecode::Instructions::Return:
		case Bytecode::Instructions::Assign:
		case Bytecode::Instructions::AssignThroughIdentifier:
		case Bytecode::Instructions::AssignSumType:
		case Bytecode::Instructions::ReadRef:
		case Bytecode::Instructions::ReadRefAnnotated:
		case Bytecode::Instructions::CopyBuffer:
		case Bytecode::Instructions::CopyStructure:
		case Bytecode::Instructions::ConstructSumType:
		case Bytecode::Instructions::TempReferenceFromRegister:
			break;

		// Single-bye operations with one payload field
		case Bytecode::Instructions::Pop:
		case Bytecode::Instructions::InvokeMeta:
		case Bytecode::Instructions::AllocStructure:
			Fetch<EpochTypeID>();
			break;

		// Operations with two payload fields
		case Bytecode::Instructions::CopyFromStructure:
		case Bytecode::Instructions::CopyToStructure:
			Fetch<StringHandle>();
			Fetch<StringHandle>();
			break;

		case Bytecode::Instructions::ReadStack:
		case Bytecode::Instructions::ReadParam:
			Fetch<size_t>();
			Fetch<size_t>();
			Fetch<size_t>();
			break;

		// Operations with string payload fields
		case Bytecode::Instructions::Read:
		case Bytecode::Instructions::InvokeIndirect:
		case Bytecode::Instructions::BindMemberRef:
		case Bytecode::Instructions::BindMemberByHandle:
		case Bytecode::Instructions::InvokeNative:
			Fetch<StringHandle>();
			break;

		case Bytecode::Instructions::BindRef:
			Fetch<size_t>();
			Fetch<size_t>();
			break;

		case Bytecode::Instructions::SetRetVal:
			Fetch<size_t>();
			break;

		// Operations we might want to muck with
		case Bytecode::Instructions::Invoke:
			{
				size_t originaloffset = InstructionOffset - 1;
				StringHandle target = Fetch<StringHandle>();

				if(OwnerVM.JITExecs.find(target) != OwnerVM.JITExecs.end())
					CodeBuffer[originaloffset] = Bytecode::Instructions::InvokeNative;
			}
			break;

		case Bytecode::Instructions::InvokeOffset:
			{
				StringHandle target = Fetch<StringHandle>();
				size_t offsetofoffset = InstructionOffset;

				Fetch<size_t>();		// Skip dummy offset

				offsetfixups[offsetofoffset] = target;
			}
			break;

		// Operations that take a bit of special processing, but we are still ignoring
		case Bytecode::Instructions::Push:
			{
				EpochTypeID pushedtype = Fetch<EpochTypeID>();
				switch(pushedtype)
				{
				case EpochType_Integer:			Fetch<Integer32>();			break;
				case EpochType_Integer16:		Fetch<Integer16>();			break;
				case EpochType_String:			Fetch<StringHandle>();		break;
				case EpochType_Boolean:			Fetch<bool>();				break;
				case EpochType_Real:			Fetch<Real32>();			break;
				case EpochType_Buffer:			Fetch<BufferHandle>();		break;
				default:						Fetch<StructureHandle>();	break;
				}
			}
			break;

		case Bytecode::Instructions::PatternMatch:
			{
				StringHandle funcname = Fetch<StringHandle>();

				offsetfixups[InstructionOffset] = funcname;

				Fetch<size_t>();
				size_t paramcount = Fetch<size_t>();
				for(size_t i = 0; i < paramcount; ++i)
				{
					EpochTypeID paramtype = Fetch<EpochTypeID>();
					bool needsmatch = (Fetch<Byte>() != 0);
					if(needsmatch)
					{
						switch(paramtype)
						{
						case EpochType_Integer:
							Fetch<Integer32>();
							break;

						default:
							throw NotImplementedException("Support for pattern matching this function parameter type is not implemented");
						}
					}
				}
			}
			break;

		case Bytecode::Instructions::TypeMatch:
			{
				StringHandle funcname = Fetch<StringHandle>();
				offsetfixups[InstructionOffset] = funcname;
				Fetch<size_t>();
				size_t paramcount = Fetch<size_t>();
				for(size_t i = 0; i < paramcount; ++i)
				{
					Fetch<bool>();
					Fetch<EpochTypeID>();
				}
			}
			break;
		
		default:
			throw FatalException("Invalid bytecode operation detected during load");
		}
	}

	// Replace function jump offsets with their true locations
	for(std::map<size_t, StringHandle>::const_iterator iter = offsetfixups.begin(); iter != offsetfixups.end(); ++iter)
	{
		size_t funcoffset = OwnerVM.GetFunctionInstructionOffset(iter->second);
		*reinterpret_cast<size_t*>(&CodeBuffer[iter->first]) = funcoffset;
	}

	// Pre-mark all statically referenced string handles
	// This helps speed up garbage collection a bit
	for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = OwnerVM.PrivateGetRawStringPool().GetInternalPool().begin(); iter != OwnerVM.PrivateGetRawStringPool().GetInternalPool().end(); ++iter)
		StaticallyReferencedStrings.insert(iter->first);

	// JIT-compile everything that needs it
	for(std::vector<StringHandle>::const_iterator iter = jitworklist.begin(); iter != jitworklist.end(); ++iter)
	{
		size_t beginoffset = entityoffsetmap[*iter];
		size_t endoffset = OwnerVM.GetEntityEndOffset(beginoffset);

		JITCompileByteCode(*iter, beginoffset, endoffset);
	}
}


//-------------------------------------------------------------------------------
// Entities
//-------------------------------------------------------------------------------

//
// Store the begin and end bytecode offsets of an entity
//
// This is used for handling custom entities; an entity's handler can instruct the VM
// to skip, execute, or even repeat an entity's code body, based on the type of entity
// involved, the expressions passed to the entity, and so on. This allows for entities
// to handle things like flow control for the language.
//
void VirtualMachine::MapEntityBeginEndOffsets(size_t beginoffset, size_t endoffset)
{
	EntityOffsets[beginoffset] = endoffset;
}

//
// Store the begin and end bytecode offsets of an entity chain
//
void VirtualMachine::MapChainBeginEndOffsets(size_t beginoffset, size_t endoffset)
{
	ChainOffsets[beginoffset] = endoffset;
}

//
// Retrieve the end offset of the entity at the specified begin offset
//
size_t VirtualMachine::GetEntityEndOffset(size_t beginoffset) const
{
	BeginEndOffsetMap::const_iterator iter = EntityOffsets.find(beginoffset);
	if(iter == EntityOffsets.end())
		throw FatalException("Failed to cache end offset of an entity, or an invalid entity begin offset was requested");

	return iter->second;
}

//
// Retrieve the end offset of the entity chain at the specified begin offset
//
size_t VirtualMachine::GetChainEndOffset(size_t beginoffset) const
{
	BeginEndOffsetMap::const_iterator iter = ChainOffsets.find(beginoffset);
	if(iter == ChainOffsets.end())
		throw FatalException("Failed to cache end offset of an entity chain, or an invalid entity chain begin offset was requested");

	return iter->second;
}

//
// Retrieve the meta control handler for an entity tag type
//
EntityMetaControl VirtualMachine::GetEntityMetaControl(Bytecode::EntityTag tag) const
{
	EntityTable::const_iterator iter = Entities.find(tag);
	if(iter != Entities.end())
		return iter->second.MetaControl;

	throw FatalException("Invalid entity type tag - no meta control could be looked up");
}


//-------------------------------------------------------------------------------
// Structure management
//-------------------------------------------------------------------------------

//
// Allocate memory for a structure instance on the freestore
//
StructureHandle VirtualMachine::AllocateStructure(const StructureDefinition &description)
{
	Threads::CriticalSection::Auto lock(StructureCritSec);

	StructureHandle handle = StructureHandleAlloc.AllocateHandle(ActiveStructures);
	ActiveStructures.insert(std::make_pair(handle, ActiveStructure(description))); 
	return handle;
}

//
// Get the definition metadata for the structure with the given type ID number
//
EPOCHVM const StructureDefinition& VirtualMachine::GetStructureDefinition(EpochTypeID type) const
{
	Threads::CriticalSection::Auto lock(StructureCritSec);

	std::map<EpochTypeID, StructureDefinition>::const_iterator iter = StructureDefinitions.find(type);
	if(iter == StructureDefinitions.end())
		throw FatalException("Invalid structure description handle");

	return iter->second;
}

//
// Look up actual structure instance data in memory given a handle
//
EPOCHVM ActiveStructure& VirtualMachine::GetStructure(StructureHandle handle)
{
	Threads::CriticalSection::Auto lock(StructureCritSec);

	std::map<StructureHandle, ActiveStructure>::iterator iter = ActiveStructures.find(handle);
	if(iter == ActiveStructures.end())
		throw FatalException("Invalid structure handle");

	return iter->second;
}

//
// Deep copy a structure and all of its contents, including strings, buffers, and other structures
//
StructureHandle VirtualMachine::DeepCopy(StructureHandle handle)
{
	Threads::CriticalSection::Auto lock(StructureCritSec);

	const ActiveStructure& original = GetStructure(handle);
	StructureHandle clonedhandle = AllocateStructure(original.Definition);
	ActiveStructure& clone = GetStructure(clonedhandle);

	for(size_t i = 0; i < original.Definition.GetNumMembers(); ++i)
	{
		EpochTypeID membertype = original.Definition.GetMemberType(i);

		if(GetTypeFamily(membertype) == Metadata::EpochTypeFamily_SumType)
		{
			membertype = original.ReadSumTypeMemberType(i);
			clone.WriteSumTypeMemberType(i, membertype);
		}

		switch(membertype)
		{
		case EpochType_Boolean:			clone.WriteMember(i, original.ReadMember<bool>(i));					break;
		case EpochType_Buffer:			clone.WriteMember(i, CloneBuffer(original.ReadMember<bool>(i)));	break;
		case EpochType_Function:		clone.WriteMember(i, original.ReadMember<StringHandle>(i));			break;
		case EpochType_Identifier:		clone.WriteMember(i, original.ReadMember<StringHandle>(i));			break;
		case EpochType_Integer:			clone.WriteMember(i, original.ReadMember<Integer32>(i));			break;
		case EpochType_Integer16:		clone.WriteMember(i, original.ReadMember<Integer16>(i));			break;
		case EpochType_Real:			clone.WriteMember(i, original.ReadMember<Real32>(i));				break;
		case EpochType_String:			clone.WriteMember(i, original.ReadMember<StringHandle>(i));			break;
		case EpochType_Nothing:																				break;

		default:
			if(GetTypeFamily(membertype) == EpochTypeFamily_Structure || GetTypeFamily(membertype) == EpochTypeFamily_TemplateInstance)
				clone.WriteMember(i, DeepCopy(original.ReadMember<StructureHandle>(i)));
			else
				throw FatalException("Invalid structure member data type; cannot deep copy");

			break;
		}
	}

	return clonedhandle;
}


//-------------------------------------------------------------------------------
// Garbage collection
//-------------------------------------------------------------------------------

//
// If the garbage collectors have ticked over, perform collection routines
//
void ExecutionContext::CollectGarbage()
{
	if(GarbageTick_Buffers > 1024)
	{
		CollectGarbage_Buffers();
		GarbageTick_Buffers = 0;
	}

	if(GarbageTick_Strings > 1024)
	{
		CollectGarbage_Strings();
		GarbageTick_Strings = 0;
	}

	if(GarbageTick_Structures > 1024)
	{
		CollectGarbage_Structures();
		GarbageTick_Structures = 0;
	}
}


//
// Tick the garbage collection counter for buffer allocations
//
EPOCHVM void ExecutionContext::TickBufferGarbageCollector()
{
	++GarbageTick_Buffers;
}

//
// Tick the garbage collection counter for string allocations
//
EPOCHVM void ExecutionContext::TickStringGarbageCollector()
{
	++GarbageTick_Strings;
}

//
// Tick the garbage collection counter for structure allocations
//
EPOCHVM void ExecutionContext::TickStructureGarbageCollector()
{
	++GarbageTick_Structures;
}



//
// Helper functions for validating if a piece of garbage is of
// the type being collected (see the MarkAndSweep function)
//
namespace
{

	bool ValidatorStrings(EpochTypeID vartype)
	{
		return (vartype == EpochType_String);
	}

	bool ValidatorBuffers(EpochTypeID vartype)
	{
		return (vartype == EpochType_Buffer);
	}

	bool ValidatorStructures(EpochTypeID vartype)
	{
		return (GetTypeFamily(vartype) == Metadata::EpochTypeFamily_Structure || GetTypeFamily(vartype) == Metadata::EpochTypeFamily_TemplateInstance);
	}

}

//
// Simple implementation of a mark-and-sweep garbage collection system
//
// Given a handle type, a validator (see above), and a set of known live handles,
// explores the freestore and stack to find all reachable (live) handles; when the
// function exits, the live handle list will contain all reachable handles which
// should NOT be garbage collected. Everything else can be freed without issues.
//
template <typename HandleType, typename ValidatorT>
void ExecutionContext::MarkAndSweep(ValidatorT validator, std::set<HandleType>& livehandles)
{
	// TODO - BUG BUG BUG - we need to handle values on the stack which are NOT bound into local variables, e.g. temporary expressions

	// Traverse the active stack, starting in the current frame and unwinding upwards to the
	// root code invocation, marking each local variable as holding the applicable reference
	ActiveScope* scope = Variables;
	while(scope)
	{
		const ScopeDescription& description = scope->GetOriginalDescription();
		size_t numvars = description.GetVariableCount();
		for(size_t i = 0; i < numvars; ++i)
		{
			EpochTypeID vartype = description.GetVariableTypeByIndex(i);
			if(validator(vartype))
			{
				HandleType marked = scope->Read<HandleType>(description.GetVariableNameHandle(i));
				livehandles.insert(marked);
			}
		}

		scope = scope->ParentScope;
	}

	// Traverse the free-store of structures, marking each applicable structure field as holding a
	// reference to the pointed-to string handle.
	for(std::map<StructureHandle, ActiveStructure>::const_iterator iter = OwnerVM.PrivateGetStructurePool().begin(); iter != OwnerVM.PrivateGetStructurePool().end(); ++iter)
	{
		const StructureDefinition& definition = iter->second.Definition;
		for(size_t i = 0; i < definition.GetNumMembers(); ++i)
		{
			if(validator(definition.GetMemberType(i)))
			{
				HandleType marked = iter->second.ReadMember<HandleType>(i);
				livehandles.insert(marked);
			}
		}
	}
}

//
// Perform garbage collection traversal for buffer data
//
void ExecutionContext::CollectGarbage_Buffers()
{
	std::set<BufferHandle> livehandles;

	// Traverse active scopes/structures for variables holding buffer references
	MarkAndSweep<BufferHandle>(ValidatorBuffers, livehandles);

	// Check the return value register to be safe
	if(ValidatorBuffers(State.ReturnValueRegister.Type))
		livehandles.insert(State.ReturnValueRegister.Value_BufferHandle);

	// Now garbage collect all buffers which are not live
	OwnerVM.GarbageCollectBuffers(livehandles);
}

//
// Perform garbage collection traversal for string data
//
void ExecutionContext::CollectGarbage_Strings()
{
	// Begin with the set of known static string references, as parsed from the code during load phase
	// This is done to ensure that statically referenced strings are never discarded by the collector.
	std::set<StringHandle> livehandles = StaticallyReferencedStrings;

	// Traverse active scopes/structures for variables holding string references
	MarkAndSweep<StringHandle>(ValidatorStrings, livehandles);

	// Check the return value register to be safe
	if(ValidatorStrings(State.ReturnValueRegister.Type))
		livehandles.insert(State.ReturnValueRegister.Value_StringHandle);

	// Now that the list of live handles is known, we can collect all unused string memory.
	OwnerVM.PrivateGetRawStringPool().GarbageCollect(livehandles);
}

//
// Perform garbage collection traversal for structure data
//
void ExecutionContext::CollectGarbage_Structures()
{
	std::set<StructureHandle> livehandles;

	// Traverse active scopes/structures for variables holding structure references
	MarkAndSweep<StructureHandle>(ValidatorStructures, livehandles);

	// Check the return value register to be safe
	if(ValidatorStructures(State.ReturnValueRegister.Type))
		livehandles.insert(State.ReturnValueRegister.Value_StructureHandle);

	// Now garbage collect all structures which are not live
	OwnerVM.GarbageCollectStructures(livehandles);
}


//
// Garbage collection disposal routine for buffer data
//
void VirtualMachine::GarbageCollectBuffers(const std::set<BufferHandle>& livehandles)
{
	EraseDeadHandles(Buffers, livehandles);
}

//
// Garbage collection disposal routine for structure data
//
void VirtualMachine::GarbageCollectStructures(const std::set<StructureHandle>& livehandles)
{
	EraseDeadHandles(ActiveStructures, livehandles);
}


//-------------------------------------------------------------------------------
// Debug hooks
//-------------------------------------------------------------------------------

//
// Generate a textual snapshot of the VM's state
//
std::wstring VirtualMachine::DebugSnapshot() const
{
	std::wostringstream report;

#ifdef EPOCHVM_VISUAL_DEBUGGER

	// Amass snapshot data of the string pool
	{
		Threads::CriticalSection::Auto lock(PrivateStringPool.CritSec);
		report << L"STRING POOL CONTENTS\r\n";
		const boost::unordered_map<StringHandle, std::wstring>& pool = PrivateStringPool.GetInternalPool();
		for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = pool.begin(); iter != pool.end(); ++iter)
			report << iter->first << L"\t" << iter->second << L"\r\n";
	}
#endif
	
	return report.str();
}



void ExecutionContext::JITCompileByteCode(StringHandle entity, size_t beginoffset, size_t endoffset)
{
	OwnerVM.JITExecs[entity] = JITByteCode(OwnerVM, CodeBuffer, beginoffset, endoffset);
}

void ExecutionContext::WriteStructureMember(ActiveStructure& structure, size_t memberindex, EpochTypeID membertype)
{
	switch(membertype)
	{
	case EpochType_Integer:
		structure.WriteMember(memberindex, State.Stack.PopValue<Integer32>());
		break;

	case EpochType_Integer16:
		structure.WriteMember(memberindex, State.Stack.PopValue<Integer16>());
		break;

	case EpochType_Boolean:
		structure.WriteMember(memberindex, State.Stack.PopValue<bool>());
		break;

	case EpochType_Buffer:
		structure.WriteMember(memberindex, State.Stack.PopValue<BufferHandle>());
		break;

	case EpochType_Real:
		structure.WriteMember(memberindex, State.Stack.PopValue<Real32>());
		break;

	case EpochType_String:
		structure.WriteMember(memberindex, State.Stack.PopValue<StringHandle>());
		break;

	case EpochType_Function:
		structure.WriteMember(memberindex, State.Stack.PopValue<StringHandle>());
		break;

	case EpochType_Nothing:
		break;

	default:
		if(GetTypeFamily(membertype) == EpochTypeFamily_SumType)
		{
			EpochTypeID actualtype = State.Stack.PopValue<EpochTypeID>();
			structure.WriteSumTypeMemberType(memberindex, actualtype);
			WriteStructureMember(structure, memberindex, actualtype);
		}
		else if(GetTypeFamily(membertype) == EpochTypeFamily_Structure || GetTypeFamily(membertype) == EpochTypeFamily_TemplateInstance)
			structure.WriteMember(memberindex, State.Stack.PopValue<StructureHandle>());
		else
			throw FatalException("Unhandled structure member type");

		break;
	}
}
