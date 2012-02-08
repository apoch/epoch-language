//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Definition of the VM wrapper class
//

#include "pch.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Virtual Machine/TypeInfo.h"
#include "Virtual Machine/Marshaling.h"

#include "Metadata/ActiveScope.h"

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
void VirtualMachine::InitStandardLibraries()
{
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochLibrary.DLL");

	typedef void (STDCALL *bindtovmptr)(FunctionInvocationTable&, EntityTable&, EntityTable&, StringPoolManager&, Bytecode::EntityTag&, EpochFunctionPtr);
	bindtovmptr bindtovm = Marshaling::DLLPool::GetFunction<bindtovmptr>(dllhandle, "BindToVirtualMachine");

	if(!bindtovm)
		throw FatalException("Failed to load Epoch standard library");

	void ExternalDispatch(StringHandle functionname, VM::ExecutionContext& context);

	Bytecode::EntityTag customtag = Bytecode::EntityTags::CustomEntityBaseID;
	bindtovm(GlobalFunctions, Entities, Entities, PrivateStringPool, customtag, ExternalDispatch);
}


//
// Execute a block of bytecode from memory
//
ExecutionResult VirtualMachine::ExecuteByteCode(const Bytecode::Instruction* buffer, size_t size)
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
ExecutionContext::ExecutionContext(VirtualMachine& ownervm, const Bytecode::Instruction* codebuffer, size_t codesize)
	: OwnerVM(ownervm),
	  CodeBuffer(codebuffer),
	  CodeBufferSize(codesize),
	  InstructionOffset(0),
	  Variables(NULL),
	  GarbageTick_Buffers(0),
	  GarbageTick_Strings(0),
	  GarbageTick_Structures(0)

{
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
	struct autoexit_scope
	{
		autoexit_scope(ExecutionContext* thisptr, ActiveScope* scope) : ThisPtr(thisptr), ScopePtr(scope) { }

		~autoexit_scope()
		{
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

		ExecutionContext* ThisPtr;
		ActiveScope* ScopePtr;
	};

	struct autoexit_functionscopes
	{
		explicit autoexit_functionscopes(ExecutionContext* thisptr) : ThisPtr(thisptr) { }

		~autoexit_functionscopes()
		{
			while(!Scopes.empty())
			{
				delete Scopes.top();
				Scopes.pop();
			}
		}

		std::stack<autoexit_scope*> Scopes;

		ExecutionContext* ThisPtr;
	};

	struct autoexit
	{
		explicit autoexit(ExecutionContext* thisptr) : ThisPtr(thisptr) { }

		~autoexit()
		{
			while(!FunctionScopes.empty())
				UnwindOutermostFunctionStack();
		}

		void SetNewFunctionUnwindPoint()
		{
			FunctionScopes.push(new autoexit_functionscopes(ThisPtr));
		}

		void UnwindOutermostFunctionStack()
		{
			if(!FunctionScopes.empty())
			{
				delete FunctionScopes.top();
				FunctionScopes.pop();
			}
		}

		void CleanUpScope(ActiveScope* scope)
		{
			if(FunctionScopes.empty())
				SetNewFunctionUnwindPoint();

			FunctionScopes.top()->Scopes.push(new autoexit_scope(ThisPtr, scope));
		}

		void CleanUpTopmostScope()
		{
			if(!FunctionScopes.empty() && !FunctionScopes.top()->Scopes.empty())
			{
				delete FunctionScopes.top()->Scopes.top();
				FunctionScopes.top()->Scopes.pop();
			}
		}

		std::stack<autoexit_functionscopes*> FunctionScopes;

		ExecutionContext* ThisPtr;
	} onexit(this);

	// By default, assume everything is alright
	State.Result.ResultType = ExecutionResult::EXEC_RESULT_OK;

	std::stack<size_t> chainoffsets;
	std::stack<bool> chainrepeats;
	std::stack<bool> returnonfunctionexitstack;

	returnonfunctionexitstack.push(returnonfunctionexit);

	try
	{
		// Run the given chunk of code
		while(State.Result.ResultType == ExecutionResult::EXEC_RESULT_OK && InstructionOffset < CodeBufferSize)
		{
			switch(CodeBuffer[InstructionOffset++])
			{
			case Bytecode::Instructions::Halt:		// Halt the machine
				State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
				break;

			case Bytecode::Instructions::NoOp:		// Do nothing for a cycle
				break;

			case Bytecode::Instructions::Return:	// Return execution control to the parent context
				State.Result.ResultType = ExecutionResult::EXEC_RESULT_RETURN;
				break;

			case Bytecode::Instructions::SetRetVal:	// Set return value register
				{
					StringHandle variablename = Fetch<StringHandle>();
					Variables->CopyToRegister(variablename, State.ReturnValueRegister);
				}
				break;

			case Bytecode::Instructions::CopyFromStructure:
				{
					StringHandle variablename = Fetch<StringHandle>();
					StringHandle membername = Fetch<StringHandle>();
					
					StructureHandle readstruct = Variables->Read<StructureHandle>(variablename);
					StringHandle actualmember = Variables->Read<StringHandle>(membername);

					ActiveStructure& structure = OwnerVM.GetStructure(readstruct);

					size_t memberindex = structure.Definition.FindMember(actualmember);
					EpochTypeID membertype = structure.Definition.GetMemberType(memberindex);
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

					default:
						if(membertype < EpochType_CustomBase)
							throw FatalException("Unhandled structure member type");

						State.ReturnValueRegister.SetStructure(structure.ReadMember<StructureHandle>(memberindex), structure.Definition.GetMemberType(memberindex));
						break;
					}
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

					default:
						if(membertype < EpochType_CustomBase)
							throw FatalException("Unhandled structure member type");

						structure.WriteMember(memberindex, State.Stack.PopValue<StructureHandle>());
						break;
					}
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
				}
				break;

			case Bytecode::Instructions::BindRef:
				{
					StringHandle target = State.Stack.PopValue<StringHandle>();
					if(Variables->GetOriginalDescription().IsReferenceByID(target))
					{
						State.Stack.PushValue(Variables->GetReferenceType(target));
						State.Stack.PushValue(Variables->GetReferenceTarget(target));
					}
					else
					{
						State.Stack.PushValue(Variables->GetOriginalDescription().GetVariableTypeByID(target));
						State.Stack.PushValue(Variables->GetVariableStorageLocation(target));
					}
				}
				break;

			case Bytecode::Instructions::BindMemberRef:
				{
					StringHandle member = Fetch<StringHandle>();

					void* storagelocation = State.Stack.PopValue<void*>();
					EpochTypeID structuretype = State.Stack.PopValue<EpochTypeID>();

					StructureHandle* phandle = reinterpret_cast<StructureHandle*>(storagelocation);
					StructureHandle handle = *phandle;

					ActiveStructure& structure = OwnerVM.GetStructure(handle);
					const StructureDefinition& definition = structure.Definition;
					size_t memberindex = definition.FindMember(member);
					size_t offset = definition.GetMemberOffset(memberindex);

					storagelocation = &(structure.Storage[0]) + offset;

					State.Stack.PushValue(definition.GetMemberType(memberindex));
					State.Stack.PushValue(storagelocation);
				}
				break;

			case Bytecode::Instructions::Pop:		// Pop some stuff off the stack
				{
					EpochTypeID poppedtype = Fetch<EpochTypeID>();
					State.Stack.Pop(GetStorageSize(poppedtype));
				}
				break;

			case Bytecode::Instructions::Read:		// Read a variable's value and place it on the stack
				{
					StringHandle variablename = Fetch<StringHandle>();
					Variables->PushOntoStack(variablename, State.Stack);
				}
				break;

			case Bytecode::Instructions::ReadRef:	// Read a reference's target value and place it on the stack
				{
					void* targetstorage = State.Stack.PopValue<void*>();
					EpochTypeID targettype = State.Stack.PopValue<EpochTypeID>();
					Variables->PushOntoStack(targetstorage, targettype, State.Stack);
				}
				break;

			case Bytecode::Instructions::Assign:	// Write a value to a variable's position on the stack
				{
					void* targetstorage = State.Stack.PopValue<void*>();
					EpochTypeID targettype = State.Stack.PopValue<EpochTypeID>();
					Variables->WriteFromStack(targetstorage, targettype, State.Stack);
				}
				break;

			case Bytecode::Instructions::AssignThroughIdentifier:
				{
					EpochTypeID targettype = State.Stack.PopValue<EpochTypeID>();
					StringHandle identifier = State.Stack.PopValue<StringHandle>();
					Variables->WriteFromStack(Variables->GetVariableStorageLocation(identifier), targettype, State.Stack);
				}
				break;

			case Bytecode::Instructions::Invoke:	// Invoke a built-in or user-defined function
				{
					StringHandle functionname = Fetch<StringHandle>();
					InvokedFunctionStack.push_back(functionname);

					size_t internaloffset = OwnerVM.GetFunctionInstructionOffsetNoThrow(functionname);
					if(internaloffset)
					{
						InstructionOffsetStack.push(InstructionOffset);

						InstructionOffset = internaloffset;
						scope = &OwnerVM.GetScopeDescription(functionname);

						returnonfunctionexitstack.push(false);
					}
					else
					{
						OwnerVM.InvokeFunction(functionname, *this);
						InvokedFunctionStack.pop_back();

						if(State.Result.ResultType == ExecutionResult::EXEC_RESULT_HALT)
							throw FatalException("Unexpected VM halt");
					}
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

			case Bytecode::Instructions::BeginEntity:
				{
					size_t originaloffset = InstructionOffset - 1;
					Bytecode::EntityTag tag = static_cast<Bytecode::EntityTag>(Fetch<Integer32>());
					StringHandle name = Fetch<StringHandle>();

					if(tag == Bytecode::EntityTags::Function)
					{
						onexit.SetNewFunctionUnwindPoint();
						Variables = new ActiveScope(*scope, Variables);
						Variables->BindParametersToStack(*this);
						Variables->PushLocalsOntoStack(*this);
						onexit.CleanUpScope(Variables);
					}
					else if(tag == Bytecode::EntityTags::FreeBlock)
					{
						scope = &OwnerVM.GetScopeDescription(name);
						Variables = new ActiveScope(*scope, Variables);
						Variables->BindParametersToStack(*this);
						Variables->PushLocalsOntoStack(*this);
						onexit.CleanUpScope(Variables);
					}
					else if(tag == Bytecode::EntityTags::Globals)
					{
						scope = &OwnerVM.GetScopeDescription(name);
						Variables = new ActiveScope(*scope, Variables);
						Variables->BindParametersToStack(*this);
						Variables->PushLocalsOntoStack(*this);
						onexit.CleanUpScope(Variables);
					}
					else if(tag == Bytecode::EntityTags::PatternMatchingResolver)
					{
						// Do nothing
					}
					else
					{
						EntityReturnCode code = OwnerVM.GetEntityMetaControl(tag)(*this);
						if(code == ENTITYRET_EXECUTE_CURRENT_LINK_IN_CHAIN)
						{
							scope = &OwnerVM.GetScopeDescription(name);
							Variables = new ActiveScope(*scope, Variables);
							Variables->BindParametersToStack(*this);
							Variables->PushLocalsOntoStack(*this);
							onexit.CleanUpScope(Variables);
						}
						else if(code == ENTITYRET_PASS_TO_NEXT_LINK_IN_CHAIN)
							InstructionOffset = OwnerVM.GetEntityEndOffset(originaloffset) + 1;
						else if(code == ENTITYRET_EXIT_CHAIN)
							InstructionOffset = OwnerVM.GetChainEndOffset(chainoffsets.top());
						else if(code == ENTITYRET_EXECUTE_AND_REPEAT_ENTIRE_CHAIN)
						{
							chainrepeats.top() = true;
							scope = &OwnerVM.GetScopeDescription(name);
							Variables = new ActiveScope(*scope, Variables);
							Variables->BindParametersToStack(*this);
							Variables->PushLocalsOntoStack(*this);
							onexit.CleanUpScope(Variables);
						}
						else
							throw FatalException("Invalid return code from entity meta-control");
					}
				}
				break;

			case Bytecode::Instructions::EndEntity:
				onexit.CleanUpTopmostScope();
				scope = &Variables->GetOriginalDescription();
				if(!chainoffsets.empty())
				{
					if(chainrepeats.top())
						InstructionOffset = chainoffsets.top() + 1;
					else
						InstructionOffset = OwnerVM.GetChainEndOffset(chainoffsets.top());
				}
				CollectGarbage();
				break;

				
			case Bytecode::Instructions::BeginChain:
				chainoffsets.push(InstructionOffset - 1);
				chainrepeats.push(false);
				break;

			case Bytecode::Instructions::EndChain:
				chainoffsets.pop();
				chainrepeats.pop();
				break;

			case Bytecode::Instructions::InvokeMeta:
				{
					Bytecode::EntityTag tag = static_cast<Bytecode::EntityTag>(Fetch<Integer32>());
					EntityReturnCode code = OwnerVM.GetEntityMetaControl(tag)(*this);

					switch(code)
					{
					case ENTITYRET_EXIT_CHAIN:
						InstructionOffset = OwnerVM.GetChainEndOffset(chainoffsets.top());
						onexit.CleanUpTopmostScope();
						scope = &Variables->GetOriginalDescription();
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
				break;


			case Bytecode::Instructions::PoolString:
				Fetch<StringHandle>();
				Fetch<std::wstring>();
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
				break;

			case Bytecode::Instructions::PatternMatch:
				{
					const char* stackptr = reinterpret_cast<const char*>(State.Stack.GetCurrentTopOfStack());
					bool matchedpattern = true;
					StringHandle originalfunction = Fetch<StringHandle>();
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
						size_t internaloffset = OwnerVM.GetFunctionInstructionOffset(originalfunction);

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
				}
				break;

			case Bytecode::Instructions::CopyBuffer:
				{
					BufferHandle originalbuffer = State.Stack.PopValue<BufferHandle>();
					BufferHandle copiedbuffer = OwnerVM.CloneBuffer(originalbuffer);
					State.Stack.PushValue(copiedbuffer);
					TickBufferGarbageCollector();
				}
				break;

			case Bytecode::Instructions::CopyStructure:
				{
					StructureHandle originalstructure = State.Stack.PopValue<StructureHandle>();
					StructureHandle copiedstructure = OwnerVM.DeepCopy(originalstructure);
					State.Stack.PushValue(copiedstructure);
					TickStructureGarbageCollector();
				}
				break;

			case Bytecode::Instructions::Tag:
				{
					Fetch<StringHandle>();		// Fetch entity to which tag is attached
					size_t tagdatacount = Fetch<size_t>();
					Fetch<std::wstring>();		// Fetch tag itself
					for(size_t i = 0; i < tagdatacount; ++i)
						Fetch<std::wstring>();	// Fetch tag data
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
				if(entitytypes.top() == Bytecode::EntityTags::Function || entitytypes.top() == Bytecode::EntityTags::PatternMatchingResolver)
					OwnerVM.AddFunction(name, originaloffset);
				entitybeginoffsets.push(originaloffset);
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

					scope.AddVariable(OwnerVM.GetPooledString(entryname), entryname, type, isreference, origin);
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
					if(type > EpochType_CustomBase)
						structdefinition = &OwnerVM.GetStructureDefinition(type);
					OwnerVM.StructureDefinitions[structuretypeid].AddMember(identifier, type, structdefinition);
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
		case Bytecode::Instructions::ReadRef:
		case Bytecode::Instructions::BindRef:
		case Bytecode::Instructions::CopyBuffer:
		case Bytecode::Instructions::CopyStructure:
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

		// Operations with string payload fields
		case Bytecode::Instructions::Read:
		case Bytecode::Instructions::Invoke:
		case Bytecode::Instructions::InvokeIndirect:
		case Bytecode::Instructions::SetRetVal:
		case Bytecode::Instructions::BindMemberRef:
			Fetch<StringHandle>();
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
				Fetch<StringHandle>();
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
		
		default:
			throw FatalException("Invalid bytecode operation detected during load");
		}
	}

	// Pre-mark all statically referenced string handles
	// This helps speed up garbage collection a bit
	for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = OwnerVM.PrivateGetRawStringPool().GetInternalPool().begin(); iter != OwnerVM.PrivateGetRawStringPool().GetInternalPool().end(); ++iter)
		StaticallyReferencedStrings.insert(iter->first);
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

		default:
			if(membertype > VM::EpochType_CustomBase)
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
		return (vartype > EpochType_CustomBase);
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

