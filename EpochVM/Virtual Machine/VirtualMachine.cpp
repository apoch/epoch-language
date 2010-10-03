//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Definition of the VM wrapper class
//

#include "pch.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Virtual Machine/TypeInfo.h"

#include "Metadata/ActiveScope.h"

#include "Bytecode/Instructions.h"
#include "Bytecode/EntityTags.h"

#include "Utility/DLLPool.h"

#include <limits>


using namespace VM;


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
		context.Execute(context.OwnerVM.GetFunctionInstructionOffset(namehandle), context.OwnerVM.GetScopeDescription(namehandle));
	}

}


//
// Initialize the bindings of standard library functions
//
void VirtualMachine::InitStandardLibraries()
{
	HINSTANCE dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochLibrary.DLL");

	typedef void (__stdcall *bindtovmptr)(FunctionInvocationTable&, StringPoolManager&);
	bindtovmptr bindtovm = reinterpret_cast<bindtovmptr>(::GetProcAddress(dllhandle, "BindToVirtualMachine"));

	if(!bindtovm)
		throw FatalException("Failed to load Epoch standard library");

	bindtovm(GlobalFunctions, StringPool);
}


//
// Execute a block of bytecode from memory
//
ExecutionResult VirtualMachine::ExecuteByteCode(const Bytecode::Instruction* buffer, size_t size)
{
	std::auto_ptr<ExecutionContext> context(new ExecutionContext(*this, buffer, size));
	context->Execute(NULL);
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
StringHandle VirtualMachine::PoolString(const std::wstring& stringdata)
{
	return StringPool.Pool(stringdata);
}

//
// Store a string in the global string pool
//
void VirtualMachine::PoolString(StringHandle handle, const std::wstring& stringdata)
{
	StringPool.Pool(handle, stringdata);
}

//
// Retrieve a pooled string from the global pool
//
const std::wstring& VirtualMachine::GetPooledString(StringHandle handle) const
{
	return StringPool.GetPooledString(handle);
}

//
// Given a pooled string value, retrieve the corresponding handle
//
StringHandle VirtualMachine::GetPooledStringHandle(const std::wstring& value)
{
	return StringPool.Pool(value);
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
	
	GlobalFunctions.insert(std::make_pair(name, FunctionInvocationHelper));
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
	std::map<StringHandle, size_t>::const_iterator iter = GlobalFunctionOffsets.find(functionname);
	if(iter == GlobalFunctionOffsets.end())
		throw InvalidIdentifierException("No function with that identifier was found");

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
	std::map<StringHandle, ScopeDescription>::const_iterator iter = LexicalScopeDescriptions.find(name);
	if(iter == LexicalScopeDescriptions.end())
		throw InvalidIdentifierException("No lexical scope has been attached to the given identifier");

	return iter->second;
}
//
// Retrieve a mutable lexical scope description
//
ScopeDescription& VirtualMachine::GetScopeDescription(StringHandle name)
{
	std::map<StringHandle, ScopeDescription>::iterator iter = LexicalScopeDescriptions.find(name);
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
	  Variables(NULL)
{
	Load();
	InstructionOffset = 0;
}

//
// Shift a function onto the call stack and invoke the corresponding code
//
void ExecutionContext::Execute(size_t offset, const ScopeDescription& scope)
{
	InstructionOffsetStack.push(InstructionOffset);

	InstructionOffset = offset;
	Execute(&scope);

	InstructionOffset = InstructionOffsetStack.top();
	InstructionOffsetStack.pop();
}

//
// Run available code until halted for some reason
//
// The reason will be provided in the attached execution context data,
// as well as any other contextual information, eg. exception payloads.
//
void ExecutionContext::Execute(const ScopeDescription* scope)
{
	// Automatically cleanup the stack if needed
	struct autoexit
	{
		autoexit(ExecutionContext* thisptr) : ThisPtr(thisptr), DoCleanup(false) { }

		~autoexit()
		{
			if(DoCleanup)
			{
				ThisPtr->Variables->PopScopeOffStack(*ThisPtr);
				bool hasreturn = ThisPtr->Variables->HasReturnVariable();
				ActiveScope* parent = ThisPtr->Variables->ParentScope;
				delete ThisPtr->Variables;
				ThisPtr->Variables = parent;
				if(hasreturn)
					ThisPtr->State.ReturnValueRegister.PushOntoStack(ThisPtr->State.Stack);
			}
		}

		ExecutionContext* ThisPtr;
		bool DoCleanup;
	} onexit(this);

	// By default, assume everything is alright
	State.Result.ResultType = ExecutionResult::EXEC_RESULT_OK;

	// Run the given chunk of code
	while(State.Result.ResultType == ExecutionResult::EXEC_RESULT_OK && InstructionOffset < CodeBufferSize)
	{
		switch(CodeBuffer[InstructionOffset++])
		{
		case Bytecode::Instructions::Halt:		// Halt the machine
			State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
			return;

		case Bytecode::Instructions::NoOp:		// Do nothing for a cycle
			break;

		case Bytecode::Instructions::Return:	// Return execution control to the parent context
			return;

		case Bytecode::Instructions::SetRetVal:	// Set return value register
			{
				StringHandle variablename = Fetch<StringHandle>();
				Variables->CopyToRegister(variablename, State.ReturnValueRegister);
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

				case EpochType_String:
					{
						StringHandle handle = Fetch<StringHandle>();
						State.Stack.PushValue(handle);
					}
					break;

				default:
					throw NotImplementedException("Cannot execute PUSH instruction: unsupported type");
				}
			}
			break;

		case Bytecode::Instructions::Read:		// Read a variable's value and place it on the stack
			{
				StringHandle variablename = Fetch<StringHandle>();
				Variables->PushOntoStack(variablename, State.Stack);
			}
			break;

		case Bytecode::Instructions::Assign:	// Write a value to a variable's position on the stack
			{
				StringHandle variablename = Fetch<StringHandle>();
				Variables->WriteFromStack(variablename, State.Stack);
			}
			break;

		case Bytecode::Instructions::Invoke:	// Invoke a built-in or user-defined function
			{
				StringHandle functionname = Fetch<StringHandle>();
				OwnerVM.InvokeFunction(functionname, *this);
			}
			break;

		case Bytecode::Instructions::BeginEntity:
			{
				Bytecode::EntityTag tag = static_cast<Bytecode::EntityTag>(Fetch<Integer32>());
				Fetch<StringHandle>();

				if(tag == Bytecode::EntityTags::Function)
				{
					Variables = new ActiveScope(*scope, Variables);
					Variables->BindParametersToStack(*this);
					Variables->PushLocalsOntoStack(*this);

					onexit.DoCleanup = true;
				}
			}
			break;

		case Bytecode::Instructions::EndEntity:
			break;


		case Bytecode::Instructions::PoolString:
			Fetch<StringHandle>();
			Fetch<std::wstring>();
			break;

		case Bytecode::Instructions::DefineLexicalScope:
			{
				Fetch<StringHandle>();
				Integer32 numentries = Fetch<Integer32>();
				for(Integer32 i = 0; i < numentries; ++i)
				{
					Fetch<StringHandle>();
					Fetch<EpochTypeID>();
					Fetch<VariableOrigin>();
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
					// Jump execution into the original function, taking care to remove the dispatcher from the call stack
					Execute(OwnerVM.GetFunctionInstructionOffset(originalfunction), OwnerVM.GetScopeDescription(originalfunction));
					return;
				}
			}
			break;
		
		default:
			throw FatalException("Invalid bytecode operation");
		}
	}
}


//
// Pre-process the bytecode stream and load certain bits of metadata needed for execution
//
// At the moment this primarily deals with loading functions from the bytecode stream and
// caching their instruction offsets so we can invoke them on demand during execution.
//
void ExecutionContext::Load()
{
	std::stack<Bytecode::EntityTag> entitytypes;

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
				if(entitytypes.top() == Bytecode::EntityTags::Function || entitytypes.top() == Bytecode::EntityTags::PatternMatchingResolver)
					OwnerVM.AddFunction(Fetch<StringHandle>(), originaloffset);
			}
			break;

		case Bytecode::Instructions::EndEntity:
			entitytypes.pop();
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
				Integer32 numentries = Fetch<Integer32>();

				OwnerVM.AddLexicalScope(scopename);
				ScopeDescription& scope = OwnerVM.GetScopeDescription(scopename);

				for(Integer32 i = 0; i < numentries; ++i)
				{
					StringHandle entryname = Fetch<StringHandle>();
					EpochTypeID type = Fetch<EpochTypeID>();
					VariableOrigin origin = Fetch<VariableOrigin>();

					scope.AddVariable(OwnerVM.GetPooledString(entryname), entryname, type, origin);
				}
			}
			break;


		// Single-byte operations with no payload
		case Bytecode::Instructions::Halt:
		case Bytecode::Instructions::NoOp:
		case Bytecode::Instructions::Return:
			break;

		// Operations with two payload fields
		case Bytecode::Instructions::Push:
			Fetch<Integer32>();
			Fetch<Integer32>();
			break;

		// Operations with string payload fields
		case Bytecode::Instructions::Read:
		case Bytecode::Instructions::Assign:
		case Bytecode::Instructions::Invoke:
		case Bytecode::Instructions::SetRetVal:
			Fetch<StringHandle>();
			break;

		// Operations that take a bit of special processing, but we are still ignoring
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
			throw FatalException("Invalid bytecode operation");
		}
	}
}

