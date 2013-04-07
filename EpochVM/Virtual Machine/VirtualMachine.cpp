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



//#define PROFILING_ENABLED
#ifdef PROFILING_ENABLED

#include "Utility/Profiling.h"
#include "User Interface/Output.h"
std::map<const char*, Profiling::Timer> GlobalTimers;

struct RAIIProfiler
{
	explicit RAIIProfiler(const char* tag)
		: Tag(tag)
	{
		GlobalTimers[tag].Begin();
	}

	~RAIIProfiler()
	{
		GlobalTimers[Tag].End();
		GlobalTimers[Tag].Accumulate();
	}

	const char* Tag;
};

#define AUTO_PROFILE(x)		RAIIProfiler raii(x)

#else

#define AUTO_PROFILE(x)

#endif

//
// Construct and initialize a virtual machine
//
// Mainly useful for forking the visual debugger thread as necessary
//
VirtualMachine::VirtualMachine()
	: EntryPointFunc(NULL),
	  GlobalInitFunc(NULL)
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
void VirtualMachine::InitStandardLibraries(ExecutionContext* context, unsigned* testharness)
{
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochLibrary.DLL");

	typedef void (STDCALL *registerlibraryptr)(FunctionSignatureSet&, StringPoolManager&);
	registerlibraryptr registerlibrary = Marshaling::DLLPool::GetFunction<registerlibraryptr>(dllhandle, "RegisterLibraryContents");

	typedef void (STDCALL *bindtovmptr)(ExecutionContext*, StringPoolManager&, JIT::JITTable&);
	bindtovmptr bindtovm = Marshaling::DLLPool::GetFunction<bindtovmptr>(dllhandle, "BindToVirtualMachine");

	if(!bindtovm || !registerlibrary)
		throw FatalException("Failed to load Epoch standard library");

	bindtovm(context, PrivateStringPool, JITHelpers);
	registerlibrary(LibraryFunctionSignatures, PrivateStringPool);

	typedef void (STDCALL *bindtotestharnessptr)(unsigned*);
	bindtotestharnessptr bindtotest = Marshaling::DLLPool::GetFunction<bindtotestharnessptr>(dllhandle, "LinkToTestHarness");

	if(!bindtotest)
		throw FatalException("Failed to load Epoch standard library - test harness init failure");

	bindtotest(testharness);
}


//
// Execute a block of bytecode from memory
//
void VirtualMachine::ExecuteByteCode(Bytecode::Instruction* buffer, size_t size, unsigned* testharness)
{
	ExecutionContext context(*this, buffer, size);
	InitStandardLibraries(&context, testharness);
	context.Load();
	context.Execute();
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

	boost::unordered_map<BufferHandle, std::vector<Byte>, fasthash>::iterator iter = Buffers.find(handle);
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

	boost::unordered_map<BufferHandle, std::vector<Byte>, fasthash>::const_iterator iter = Buffers.find(handle);
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

	boost::unordered_map<BufferHandle, std::vector<Byte>, fasthash>::const_iterator iter = Buffers.find(handle);
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
	if(GlobalFunctionOffsets.find(name) != GlobalFunctionOffsets.end())
		throw InvalidIdentifierException("Function identifier is already in use");

	GlobalFunctionOffsets.insert(std::make_pair(name, instructionoffset));
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
	  GarbageTick_Buffers(0),
	  GarbageTick_Strings(0),
	  GarbageTick_Structures(0)

{
}

void ExecutionContext::Execute()
{
	void SetGlobalExecutionContext(VM::ExecutionContext* context);
	SetGlobalExecutionContext(this);

	typedef void (*pfunc)();
	if(OwnerVM.GlobalInitFunc)
		((pfunc)(OwnerVM.GlobalInitFunc))();
	else if(OwnerVM.EntryPointFunc)
		((pfunc)(OwnerVM.EntryPointFunc))();
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
	size_t globalentityoffset = 0;

	std::set<StringHandle> jitworklist;
	std::map<StringHandle, size_t> entityoffsetmap;

	std::map<size_t, StringHandle> offsetfixups;
	std::map<size_t, StringHandle> jitfixups;

	StringHandle typematcherid = 0;
	StringHandle patternmatcherid = 0;

	size_t instructionoffset = 0;
	while(instructionoffset < CodeBufferSize)
	{
		Bytecode::Instruction instruction = CodeBuffer[instructionoffset++];
		switch(instruction)
		{
		// Operations we care about
		case Bytecode::Instructions::BeginEntity:
			{
				size_t originaloffset = instructionoffset - 1;
				entitytypes.push(Fetch<Integer32>(instructionoffset));
				StringHandle name = Fetch<StringHandle>(instructionoffset);
				if(
					  entitytypes.top() == Bytecode::EntityTags::Function
				   || entitytypes.top() == Bytecode::EntityTags::PatternMatchingResolver
				   || entitytypes.top() == Bytecode::EntityTags::TypeResolver
				  )
				{
					OwnerVM.AddFunction(name, originaloffset);
					jitworklist.insert(name);
				}

				if(entitytypes.top() == Bytecode::EntityTags::TypeResolver)
					typematcherid = name;
				else
					typematcherid = 0;

				if(entitytypes.top() == Bytecode::EntityTags::PatternMatchingResolver)
					patternmatcherid = name;
				else
					patternmatcherid = 0;

				if(entitytypes.top() == Bytecode::EntityTags::Globals)
					globalentityoffset = originaloffset;

				entitybeginoffsets.push(originaloffset);
				entityoffsetmap[name] = originaloffset;
			}
			break;

		case Bytecode::Instructions::EndEntity:
			if(typematcherid)
				jitworklist.insert(typematcherid);

			if(patternmatcherid)
				jitworklist.insert(patternmatcherid);

			entitytypes.pop();
			OwnerVM.MapEntityBeginEndOffsets(entitybeginoffsets.top(), instructionoffset - 1);
			entitybeginoffsets.pop();
			break;

		case Bytecode::Instructions::BeginChain:
			{
				size_t originaloffset = instructionoffset - 1;
				chainbeginoffsets.push(originaloffset);
			}
			break;

		case Bytecode::Instructions::EndChain:
			OwnerVM.MapChainBeginEndOffsets(chainbeginoffsets.top(), instructionoffset - 1);
			chainbeginoffsets.pop();
			break;

		case Bytecode::Instructions::PoolString:
			{
				StringHandle handle = Fetch<StringHandle>(instructionoffset);
				std::wstring strvalue = Fetch<std::wstring>(instructionoffset);
				OwnerVM.PoolString(handle, strvalue);
			}
			break;

		case Bytecode::Instructions::DefineLexicalScope:
			{
				StringHandle scopename = Fetch<StringHandle>(instructionoffset);
				StringHandle parentscopename = Fetch<StringHandle>(instructionoffset);
				Integer32 numentries = Fetch<Integer32>(instructionoffset);

				OwnerVM.AddLexicalScope(scopename);
				ScopeDescription& scope = OwnerVM.GetScopeDescription(scopename);
				if(parentscopename)
					scope.ParentScope = &OwnerVM.GetScopeDescription(parentscopename);

				for(Integer32 i = 0; i < numentries; ++i)
				{
					StringHandle entryname = Fetch<StringHandle>(instructionoffset);
					EpochTypeID type = Fetch<EpochTypeID>(instructionoffset);
					VariableOrigin origin = Fetch<VariableOrigin>(instructionoffset);
					bool isreference = Fetch<bool>(instructionoffset);

					scope.AddVariable(OwnerVM.GetPooledString(entryname), entryname, 0, type, isreference, origin);
				}
			}
			break;

		case Bytecode::Instructions::DefineStructure:
			{
				EpochTypeID structuretypeid = Fetch<EpochTypeID>(instructionoffset);
				size_t numentries = Fetch<size_t>(instructionoffset);

				for(size_t i = 0; i < numentries; ++i)
				{
					StringHandle identifier = Fetch<StringHandle>(instructionoffset);
					EpochTypeID type = Fetch<EpochTypeID>(instructionoffset);
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
				EpochTypeID sumtypeid = Fetch<EpochTypeID>(instructionoffset);
				size_t numentries = Fetch<size_t>(instructionoffset);

				for(size_t i = 0; i < numentries; ++i)
				{
					EpochTypeID basetype = Fetch<EpochTypeID>(instructionoffset);
					size_t basetypesize = GetStorageSize(basetype);
					OwnerVM.VariantDefinitions[sumtypeid].AddBaseType(basetype, basetypesize);
				}
			}
			break;

		case Bytecode::Instructions::Tag:
			{
				StringHandle entity = Fetch<StringHandle>(instructionoffset);
				size_t tagdatacount = Fetch<size_t>(instructionoffset);
				std::vector<std::wstring> metadata;
				std::wstring tag = Fetch<std::wstring>(instructionoffset);
				for(size_t i = 0; i < tagdatacount; ++i)
					metadata.push_back(Fetch<std::wstring>(instructionoffset));

				if(tag == L"external")
				{
					if(tagdatacount != 2)
						throw FatalException("Incorrect number of metadata tag parameters for external marshaled function");

					RegisterMarshaledExternalFunction(entity, metadata[0], metadata[1]);
				}
				else if(tag == L"@@autogen@constructor")
				{
					if(tagdatacount != 0)
						throw FatalException("Unexpected metadata for autogenerated constructor tag");

					OwnerVM.AutoGeneratedConstructors.insert(entity);
				}
				else
					throw FatalException("Unrecognized function tag in bytecode");
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
			Fetch<size_t>(instructionoffset);
			break;

		case Bytecode::Instructions::InvokeMeta:
		case Bytecode::Instructions::AllocStructure:
			Fetch<EpochTypeID>(instructionoffset);
			break;

		// Operations with two payload fields
		case Bytecode::Instructions::CopyFromStructure:
		case Bytecode::Instructions::CopyToStructure:
			Fetch<StringHandle>(instructionoffset);
			Fetch<StringHandle>(instructionoffset);
			break;

		case Bytecode::Instructions::ReadStack:
		case Bytecode::Instructions::ReadParam:
			Fetch<size_t>(instructionoffset);
			Fetch<size_t>(instructionoffset);
			Fetch<size_t>(instructionoffset);
			break;

		// Operations with string payload fields
		case Bytecode::Instructions::Read:
		case Bytecode::Instructions::InvokeIndirect:
		case Bytecode::Instructions::BindMemberByHandle:
			Fetch<StringHandle>(instructionoffset);
			break;

		case Bytecode::Instructions::BindMemberRef:
			Fetch<EpochTypeID>(instructionoffset);
			Fetch<size_t>(instructionoffset);
			break;

		case Bytecode::Instructions::InvokeNative:
			Fetch<StringHandle>(instructionoffset);
			break;

		case Bytecode::Instructions::BindRef:
			Fetch<size_t>(instructionoffset);
			Fetch<size_t>(instructionoffset);
			break;

		case Bytecode::Instructions::SetRetVal:
			Fetch<size_t>(instructionoffset);
			break;

		// Operations we might want to muck with
		case Bytecode::Instructions::Invoke:
			Fetch<StringHandle>(instructionoffset);
			break;

		case Bytecode::Instructions::InvokeOffset:
			{
				size_t originaloffset = instructionoffset - 1;
				StringHandle target = Fetch<StringHandle>(instructionoffset);
				size_t offsetofoffset = instructionoffset;

				Fetch<size_t>(instructionoffset);		// Skip dummy offset

				offsetfixups[offsetofoffset] = target;
				jitfixups[originaloffset] = target;
			}
			break;

		// Operations that take a bit of special processing, but we are still ignoring
		case Bytecode::Instructions::Push:
			{
				EpochTypeID pushedtype = Fetch<EpochTypeID>(instructionoffset);
				switch(pushedtype)
				{
				case EpochType_Integer:			Fetch<Integer32>(instructionoffset);		break;
				case EpochType_Integer16:		Fetch<Integer16>(instructionoffset);		break;
				case EpochType_String:			Fetch<StringHandle>(instructionoffset);		break;
				case EpochType_Boolean:			Fetch<bool>(instructionoffset);				break;
				case EpochType_Real:			Fetch<Real32>(instructionoffset);			break;
				case EpochType_Buffer:			Fetch<BufferHandle>(instructionoffset);		break;
				default:						Fetch<StructureHandle>(instructionoffset);	break;
				}
			}
			break;

		case Bytecode::Instructions::PatternMatch:
			{
				StringHandle funcname = Fetch<StringHandle>(instructionoffset);

				offsetfixups[instructionoffset] = funcname;

				Fetch<size_t>(instructionoffset);
				size_t paramcount = Fetch<size_t>(instructionoffset);
				for(size_t i = 0; i < paramcount; ++i)
				{
					EpochTypeID paramtype = Fetch<EpochTypeID>(instructionoffset);
					bool needsmatch = (Fetch<Byte>(instructionoffset) != 0);
					if(needsmatch)
					{
						switch(paramtype)
						{
						case EpochType_Integer:
							Fetch<Integer32>(instructionoffset);
							break;

						default:
							throw NotImplementedException("Support for pattern matching this function parameter type is not implemented");
						}
					}
				}

				OwnerVM.PatternMatcherParamCount[patternmatcherid] = paramcount;
				OwnerVM.PatternMatcherRetType[patternmatcherid] = OwnerVM.GetScopeDescription(funcname).GetReturnVariableType();

				if(!OwnerVM.PatternMatcherDispatchHint[patternmatcherid])
					OwnerVM.PatternMatcherDispatchHint[patternmatcherid] = funcname;
			}
			break;

		case Bytecode::Instructions::TypeMatch:
			{
				StringHandle funcname = Fetch<StringHandle>(instructionoffset);
				offsetfixups[instructionoffset] = funcname;
				Fetch<size_t>(instructionoffset);
				size_t paramcount = Fetch<size_t>(instructionoffset);
				for(size_t i = 0; i < paramcount; ++i)
				{
					Fetch<bool>(instructionoffset);
					Fetch<EpochTypeID>(instructionoffset);
				}

				if(OwnerVM.TypeMatcherRetType.find(typematcherid) == OwnerVM.TypeMatcherRetType.end())
				{
					OwnerVM.TypeMatcherParamCount[typematcherid] = paramcount;
					OwnerVM.TypeMatcherRetType[typematcherid] = OwnerVM.GetScopeDescription(funcname).GetReturnVariableType();
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

	// Fixup non-typematch function invokes to native invokes
	for(std::map<size_t, StringHandle>::const_iterator iter = jitfixups.begin(); iter != jitfixups.end(); ++iter)
	{
		if(
			OwnerVM.TypeMatcherParamCount.find(iter->second) == OwnerVM.TypeMatcherParamCount.end()
	     && OwnerVM.PatternMatcherParamCount.find(iter->second) == OwnerVM.PatternMatcherParamCount.end()
		  )
		{
			CodeBuffer[iter->first] = Bytecode::Instructions::InvokeNative;
		}
	}

	// Pre-mark all statically referenced string handles
	// This helps speed up garbage collection a bit
	for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = OwnerVM.PrivateGetRawStringPool().GetInternalPool().begin(); iter != OwnerVM.PrivateGetRawStringPool().GetInternalPool().end(); ++iter)
		StaticallyReferencedStrings.insert(iter->first);

	// JIT-compile everything that needs it
	{
		JIT::NativeCodeGenerator jitgen(OwnerVM, CodeBuffer);

		if(globalentityoffset)
			jitgen.AddGlobalEntity(globalentityoffset);

		for(std::set<StringHandle>::const_iterator iter = jitworklist.begin(); iter != jitworklist.end(); ++iter)
		{
			size_t beginoffset = entityoffsetmap[*iter];
			size_t endoffset = OwnerVM.GetEntityEndOffset(beginoffset);

			jitgen.AddFunction(beginoffset, endoffset, *iter);
		}

		if(!jitworklist.empty())
			jitgen.Generate();
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


//-------------------------------------------------------------------------------
// Structure management
//-------------------------------------------------------------------------------

//
// Allocate memory for a structure instance on the freestore
//
StructureHandle VirtualMachine::AllocateStructure(const StructureDefinition& description)
{
	//Threads::CriticalSection::Auto lock(StructureCritSec);

	ActiveStructure* active = new ActiveStructure(description);
	StructureHandle handle = &active->Storage[0];
	ActiveStructures.insert(std::make_pair(handle, active)); 
	return handle;
}

ActiveStructure& VirtualMachine::FindStructureMetadata(StructureHandle handle)
{
	boost::unordered_map<StructureHandle, ActiveStructure*>::iterator iter = ActiveStructures.find(handle);
	if(iter == ActiveStructures.end())
		throw FatalException("Invalid structure handle or no metadata cached");

	return *iter->second;
}

//
// Get the definition metadata for the structure with the given type ID number
//
EPOCHVM const StructureDefinition& VirtualMachine::GetStructureDefinition(EpochTypeID type) const
{
	//Threads::CriticalSection::Auto lock(StructureCritSec);

	std::map<EpochTypeID, StructureDefinition>::const_iterator iter = StructureDefinitions.find(type);
	if(iter == StructureDefinitions.end())
		throw FatalException("Invalid structure description handle");

	return iter->second;
}

//
// Deep copy a structure and all of its contents, including strings, buffers, and other structures
//
StructureHandle VirtualMachine::DeepCopy(StructureHandle handle)
{
	//Threads::CriticalSection::Auto lock(StructureCritSec);

	const ActiveStructure& original = FindStructureMetadata(handle);
	StructureHandle clonedhandle = AllocateStructure(original.Definition);
	ActiveStructure& clone = FindStructureMetadata(clonedhandle);

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
void ExecutionContext::MarkAndSweep(ValidatorT validator, boost::unordered_set<HandleType>& livehandles)
{
	// TODO - reimplement garbage collector for JITter
	((void)(validator));
	((void)(livehandles));
}

//
// Perform garbage collection traversal for buffer data
//
void ExecutionContext::CollectGarbage_Buffers()
{
	boost::unordered_set<BufferHandle> livehandles;

	// Traverse active scopes/structures for variables holding buffer references
	MarkAndSweep<BufferHandle>(ValidatorBuffers, livehandles);

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
	boost::unordered_set<StringHandle> livehandles = StaticallyReferencedStrings;

	// Traverse active scopes/structures for variables holding string references
	MarkAndSweep<StringHandle>(ValidatorStrings, livehandles);

	// Now that the list of live handles is known, we can collect all unused string memory.
	OwnerVM.PrivateGetRawStringPool().GarbageCollect(livehandles);
}

//
// Perform garbage collection traversal for structure data
//
void ExecutionContext::CollectGarbage_Structures()
{
	boost::unordered_set<StructureHandle> livehandles;

	// Traverse active scopes/structures for variables holding structure references
	MarkAndSweep<StructureHandle>(ValidatorStructures, livehandles);

	// Now garbage collect all structures which are not live
	OwnerVM.GarbageCollectStructures(livehandles);
}


//
// Garbage collection disposal routine for buffer data
//
void VirtualMachine::GarbageCollectBuffers(const boost::unordered_set<BufferHandle>& livehandles)
{
	EraseDeadHandles(Buffers, livehandles);
}

//
// Garbage collection disposal routine for structure data
//
void VirtualMachine::GarbageCollectStructures(const boost::unordered_set<StructureHandle>& livehandles)
{
	EraseAndDeleteDeadHandles(ActiveStructures, livehandles);
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

