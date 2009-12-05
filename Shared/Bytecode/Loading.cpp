//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Functions for converting binary opcodes into VM objects
//

#include "pch.h"

#include "Bytecode/Loading.h"
#include "Bytecode/Bytecode.h"
#include "Bytecode/BytecodeExceptions.h"

#include "Virtual Machine/Operations/Debugging.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Variables/TupleVariable.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"

#include "Virtual Machine/Operations/Flow/FlowControl.h"
#include "Virtual Machine/Operations/Flow/Invoke.h"
#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Operations/Operators/Comparison.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Variables/TupleOps.h"
#include "Virtual Machine/Operations/Variables/StructureOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Operators/Bitwise.h"
#include "Virtual Machine/Operations/Operators/Logical.h"
#include "Virtual Machine/Operations/Variables/StringOps.h"
#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/Concurrency/FutureOps.h"
#include "Virtual Machine/Operations/Containers/MapReduce.h"

#include "Virtual Machine/Types Management/RuntimeCasts.h"
#include "Virtual Machine/Types Management/TypeInfo.h"

#include "Virtual Machine/Operations/Concurrency/Tasks.h"
#include "Virtual Machine/Operations/Concurrency/Messaging.h"
#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"

#include "Virtual Machine/SelfAware.inl"

#include "Marshalling/ExternalDLL.h"

#include "Utility/Strings.h"


//
// Construct and initialize the loader
//
// This function also begins the conversion operation. Conversion consists
// of two steps. In the prepass step, all global variables and functions
// are loaded and registered. In the second pass, bytecode is converted to
// internal operation objects, using the namespace information gleaned from
// the first pass.
//
FileLoader::FileLoader(const void* buffer)
	: Buffer(reinterpret_cast<const UByte*>(buffer)),
	  Offset(0),
	  LoadingProgram(new VM::Program),
	  IsPrepass(true)
{
	try
	{
		CheckCookie();
		LoadScope(true);

		Offset = 0;
		IsPrepass = false;
		CheckCookie();
		LoadScope(true);

		LoadGlobalInitBlock();
	}
	catch(...)
	{
		Clean();
		throw;
	}
}

//
// Destruct and clean up the loader
//
FileLoader::~FileLoader()
{
	Clean();
}

//
// Actually free memory used by the loader
//
void FileLoader::Clean()
{
	delete LoadingProgram;

	for(std::set<VM::ScopeDescription*>::iterator iter = DeleteScopes.begin(); iter != DeleteScopes.end(); ++iter)
		delete *iter;
	DeleteScopes.clear();
}


//
// Ensure the binary cookie is correct
//
void FileLoader::CheckCookie()
{
	if(memcmp(Buffer + Offset, Bytecode::HeaderCookie, strlen(Bytecode::HeaderCookie)) != 0)
		throw InvalidBytecodeException("Binary code does not contain a valid signature cookie; this may indicate a corrupted binary or an outdated library");

	Offset += strlen(Bytecode::HeaderCookie);
}

//
// Ensure the expected instruction is coming up
//
void FileLoader::ExpectInstruction(unsigned char instruction)
{
	const UByte* p = Buffer + Offset;
	if(*p != instruction)
		throw InvalidBytecodeException("Expected a specific instruction, but a different instruction was found; ensure the binary is not corrupted");
	++Offset;
}

//
// Read a 32-bit number from the buffer
//
Integer32 FileLoader::ReadNumber()
{
	const UByte* p = Buffer + Offset;
	Integer32 value = *reinterpret_cast<const Integer32*>(p);
	Offset += sizeof(Integer32);
	return value;
}

//
// Read a 32-bit float from the buffer
//
Real FileLoader::ReadFloat()
{
	const UByte* p = Buffer + Offset;
	Real value = *reinterpret_cast<const Real*>(p);
	Offset += sizeof(Real);
	return value;
}

//
// Read a byte-size flag from the buffer
//
bool FileLoader::ReadFlag()
{
	const UByte* p = Buffer + Offset;
	char value = *reinterpret_cast<const Byte*>(p);
	Offset += sizeof(Byte);
	return (value ? true : false);
}

//
// Read a null-terminated string
//
std::string FileLoader::ReadNullTerminatedString()
{
	std::string ret;
	const UByte* p = Buffer + Offset;
	const Byte* pchar = reinterpret_cast<const Byte*>(p);
	while(*pchar)
	{
		ret.push_back(*pchar);
		++pchar;
		++Offset;
	}

	++Offset;
	return ret;
}

//
// Read a string of a known length
//
std::string FileLoader::ReadStringByLength(Integer32 len)
{
	std::string ret;
	const UByte* p = Buffer + Offset;
	const Byte* pchar = reinterpret_cast<const Byte*>(p);
	for(Integer32 i = 0; i < len; ++i)
	{
		ret.push_back(*pchar);
		++pchar;
		++Offset;
	}

	return ret;
}

//
// Read a single instruction
//
unsigned char FileLoader::ReadInstruction()
{
	unsigned char ret = *(Buffer + Offset);
	Offset += sizeof(unsigned char);
	return ret;
}

//
// Read an instruction without removing it
// from the queue of code to be loaded
//
unsigned char FileLoader::PeekInstruction()
{
	return *(Buffer + Offset);
}


//
// Load data comprising a lexical scope
//
VM::ScopeDescription* FileLoader::LoadScope(bool linktoglobal)
{
	ExpectInstruction(Bytecode::Scope);
	ScopeID scopeid = ReadNumber();
	if(linktoglobal)
		ScopeIDMap[scopeid] = &LoadingProgram->GetGlobalScope();
	else
	{
		if(IsPrepass)
			ScopeIDMap[scopeid] = RegisterScopeToDelete(new VM::ScopeDescription);
	}

	ExpectInstruction(Bytecode::ParentScope);
	ScopeID parentscopeid = ReadNumber();
	if(parentscopeid && !IsPrepass)
		ScopeIDMap[scopeid]->ParentScope = ScopeIDMap.find(parentscopeid)->second;

	ExpectInstruction(Bytecode::Variables);
	Integer32 numvars = ReadNumber();
	for(Integer32 i = 0; i < numvars; ++i)
	{
		bool isreference = ReadFlag();
		std::string varname = ReadNullTerminatedString();
		Integer32 vartype = ReadNumber();

		if(!IsPrepass)
		{
			if(isreference)
				ScopeIDMap[scopeid]->AddReference(static_cast<VM::EpochVariableTypeID>(vartype), WidenAndCache(varname));
			else
			{
				if(vartype == VM::EpochVariableType_Tuple)
				{
					ScopeIDMap[scopeid]->Variables.insert(VM::ScopeDescription::VariableMapEntry(WidenAndCache(varname), VM::TupleVariable(NULL)));
					ScopeIDMap[scopeid]->MemberOrder.push_back(WidenAndCache(varname));
				}
				else if(vartype == VM::EpochVariableType_Structure)
				{
					ScopeIDMap[scopeid]->Variables.insert(VM::ScopeDescription::VariableMapEntry(WidenAndCache(varname), VM::StructureVariable(NULL)));
					ScopeIDMap[scopeid]->MemberOrder.push_back(WidenAndCache(varname));
				}
				else if(vartype == VM::EpochVariableType_Function)
					ScopeIDMap[scopeid]->MemberOrder.push_back(WidenAndCache(varname));
				else
					ScopeIDMap[scopeid]->AddVariable(WidenAndCache(varname), static_cast<VM::EpochVariableTypeID>(vartype));
			}
		}
	}

	ExpectInstruction(Bytecode::Ghosts);
	Integer32 numghosts = ReadNumber();
	for(Integer32 i = 0; i < numghosts; ++i)
	{
		ExpectInstruction(Bytecode::GhostRecord);

		if(!IsPrepass)
			ScopeIDMap[scopeid]->Ghosts.push_back(VM::ScopeDescription::GhostVariableMap());

		Integer32 numrecs = ReadNumber();
		for(Integer32 j = 0; j < numrecs; ++j)
		{
			std::string varname = ReadNullTerminatedString();
			ScopeID ownerid = ReadNumber();

			if(!IsPrepass)
				ScopeIDMap[scopeid]->Ghosts.back().insert(VM::ScopeDescription::GhostVariableMapEntry(WidenAndCache(varname), ScopeIDMap.find(ownerid)->second));
		}
	}

	ExpectInstruction(Bytecode::Functions);
	Integer32 numfuncs = ReadNumber();
	for(Integer32 i = 0; i < numfuncs; ++i)
	{
		std::string funcname = ReadNullTerminatedString();
		Integer32 funcid = ReadNumber();
		ReadNumber();

		unsigned char nextop = PeekInstruction();
		if(nextop == Bytecode::CallDLL)
		{
			ReadInstruction();
			std::string dllname = ReadNullTerminatedString();
			std::string dllfuncname = ReadNullTerminatedString();
			Integer32 returntype = ReadNumber();

			VM::ScopeDescription* params = LoadScope(false);
			if(IsPrepass)
			{
				UnregisterScopeToDelete(params);
				std::auto_ptr<VM::FunctionBase> callop(new Marshalling::CallDLL(WidenAndCache(dllname), WidenAndCache(dllfuncname), params, static_cast<VM::EpochVariableTypeID>(returntype)));
				FunctionIDMap[funcid] = callop.get();
				ScopeIDMap[scopeid]->AddFunction(WidenAndCache(funcname), callop);
			}
		}
		else
		{
			VM::ScopeDescription* params = LoadScope(false);
			VM::ScopeDescription* returns = LoadScope(false);
			ExpectInstruction(Bytecode::BeginBlock);
			VM::ScopeDescription* localscope = LoadScope(false);
			VM::Block* codeblock = LoadCodeBlock();
			if(IsPrepass)
			{
				std::auto_ptr<VM::FunctionBase> func(new VM::Function(NULL, params, returns));
				FunctionIDMap[funcid] = func.get();
				ScopeIDMap[scopeid]->AddFunction(WidenAndCache(funcname), func);
				UnregisterScopeToDelete(params);
				UnregisterScopeToDelete(returns);
			}
			else
			{
				codeblock->BindToScope(UnregisterScopeToDelete(localscope));
				dynamic_cast<VM::Function*>(FunctionIDMap[funcid])->SetCodeBlock(codeblock);
			}
		}
	}

	ExpectInstruction(Bytecode::FunctionSignatureList);
	Integer32 numfuncsignatures = ReadNumber();
	for(Integer32 i = 0; i < numfuncsignatures; ++i)
	{
		std::string signaturename = ReadNullTerminatedString();
		ExpectInstruction(Bytecode::FunctionSignatureBegin);
		VM::FunctionSignature signature = LoadFunctionSignature();
		if(!IsPrepass)
			ScopeIDMap[scopeid]->AddFunctionSignature(WidenAndCache(signaturename), signature, false);
	}

	ExpectInstruction(Bytecode::TupleTypes);
	Integer32 numtupletypes = ReadNumber();
	for(Integer32 i = 0; i < numtupletypes; ++i)
	{
		std::string varname = ReadNullTerminatedString();
		TupleTypeID id = ReadNumber();

		if(!IsPrepass)
			ScopeIDMap[scopeid]->TupleTypes.insert(VM::ScopeDescription::TupleTypeIDMapEntry(WidenAndCache(varname), id));
	}

	ExpectInstruction(Bytecode::TupleHints);
	Integer32 numtuplehints = ReadNumber();
	for(Integer32 i = 0; i < numtuplehints; ++i)
	{
		std::string varname = ReadNullTerminatedString();
		TupleTypeID hint = ReadNumber();
		
		if(!IsPrepass)
			ScopeIDMap[scopeid]->TupleTypeHints.insert(VM::ScopeDescription::TupleTypeIDMapEntry(WidenAndCache(varname), hint));
	}

	ExpectInstruction(Bytecode::TupleTypeMap);
	Integer32 numtupledata = ReadNumber();
	for(Integer32 i = 0; i < numtupledata; ++i)
	{
		TupleTypeID id = ReadNumber();
		ExpectInstruction(Bytecode::Members);

		std::auto_ptr<VM::TupleType> tupletype(NULL);
		if(!IsPrepass)
			tupletype.reset(new VM::TupleType);

		Integer32 nummembers = ReadNumber();
		for(Integer32 j = 0; j < nummembers; ++j)
		{
			std::string membername = ReadNullTerminatedString();
			Integer32 type = ReadNumber();
			Integer32 offset = ReadNumber();

			if(!IsPrepass)
				tupletype->AddMember(WidenAndCache(membername), static_cast<VM::EpochVariableTypeID>(type));
		}

		if(!IsPrepass)
		{
			tupletype->ComputeOffsets(*ScopeIDMap[scopeid]);
			ScopeIDMap[scopeid]->TupleTracker.TupleTypeMap.insert(std::make_pair(id, tupletype.release()));
			VM::TupleTrackerClass::OwnerMap.insert(std::make_pair(id, &ScopeIDMap[scopeid]->TupleTracker));
		}
	}

	ExpectInstruction(Bytecode::StructureTypes);
	Integer32 numstructtypes = ReadNumber();
	for(Integer32 i = 0; i < numstructtypes; ++i)
	{
		std::string varname = ReadNullTerminatedString();
		StructureTypeID id = ReadNumber();

		if(!IsPrepass)
			ScopeIDMap[scopeid]->StructureTypes.insert(VM::ScopeDescription::StructureTypeIDMapEntry(WidenAndCache(varname), id));
	}

	ExpectInstruction(Bytecode::StructureHints);
	Integer32 numstructhints = ReadNumber();
	for(Integer32 i = 0; i < numstructhints; ++i)
	{
		std::string varname = ReadNullTerminatedString();
		StructureTypeID hint = ReadNumber();

		if(!IsPrepass)
			ScopeIDMap[scopeid]->StructureTypeHints.insert(VM::ScopeDescription::StructureTypeIDMapEntry(WidenAndCache(varname), hint));
	}

	ExpectInstruction(Bytecode::StructureTypeMap);
	Integer32 numstructdata = ReadNumber();
	for(Integer32 i = 0; i < numstructdata; ++i)
	{
		StructureTypeID id = ReadNumber();
		ExpectInstruction(Bytecode::Members);

		std::auto_ptr<VM::StructureType> structtype(NULL);
		if(!IsPrepass)
			structtype.reset(new VM::StructureType);

		Integer32 nummembers = ReadNumber();
		for(Integer32 j = 0; j < nummembers; ++j)
		{
			std::string membername = ReadNullTerminatedString();
			Integer32 type = ReadNumber();
			Integer32 offset = ReadNumber();
			StructureTypeID hint = 0;

			if(type == VM::EpochVariableType_Structure || type == VM::EpochVariableType_Tuple)
				hint = ReadNumber();

			if(!IsPrepass)
			{
				if(type == VM::EpochVariableType_Structure)
					structtype->AddMember(WidenAndCache(membername), VM::StructureTrackerClass::GetOwnerOfStructureType(hint)->GetStructureType(hint), hint);
				else if(type == VM::EpochVariableType_Tuple)
					structtype->AddMember(WidenAndCache(membername), VM::TupleTrackerClass::GetOwnerOfTupleType(hint)->GetTupleType(hint), hint);
				else
					structtype->AddMember(WidenAndCache(membername), static_cast<VM::EpochVariableTypeID>(type));
			}
		}

		if(!IsPrepass)
		{
			structtype->ComputeOffsets(*ScopeIDMap[scopeid]);
			ScopeIDMap[scopeid]->StructureTracker.StructureTypeMap.insert(std::make_pair(id, structtype.release()));
			VM::StructureTrackerClass::OwnerMap.insert(std::make_pair(id, &ScopeIDMap[scopeid]->StructureTracker));
		}
	}

	ExpectInstruction(Bytecode::Constants);

	UINT_PTR numconstants = ReadNumber();
	for(UINT_PTR i = 0; i < numconstants; ++i)
		ScopeIDMap[scopeid]->SetConstant(WidenAndCache(ReadNullTerminatedString()));


	ExpectInstruction(Bytecode::ResponseMaps);

	UINT_PTR numresponsemaps = ReadNumber();
	for(UINT_PTR i = 0; i < numresponsemaps; ++i)
	{
		std::wstring mapname = WidenAndCache(ReadNullTerminatedString());
		UINT_PTR nummapentries = ReadNumber();
		std::auto_ptr<VM::ResponseMap> themap(new VM::ResponseMap);
		for(UINT_PTR j = 0; j < nummapentries; ++j)
		{
			std::string messagename = ReadNullTerminatedString();
			UINT_PTR nummessageparams = ReadNumber();
			std::list<VM::EpochVariableTypeID> paramtypes;
			for(UINT_PTR k = 0; k < nummessageparams; ++k)
			{
				VM::EpochVariableTypeID paramtype = static_cast<VM::EpochVariableTypeID>(ReadNumber());
				paramtypes.push_back(paramtype);
			}

			ExpectInstruction(Bytecode::BeginBlock);
			VM::ScopeDescription* responsescope = LoadScope(false);
			std::auto_ptr<VM::Block> responseblock(LoadCodeBlock());
			VM::ScopeDescription* auxscope = LoadScope(false);

			if(!IsPrepass)
			{
				responseblock->BindToScope(UnregisterScopeToDelete(responsescope));
				std::auto_ptr<VM::ResponseMapEntry> mapentry(new VM::ResponseMapEntry(WidenAndCache(messagename), paramtypes, responseblock.release(), UnregisterScopeToDelete(auxscope)));
				themap->AddEntry(mapentry.release());
			}
		}
		
		if(!IsPrepass)
			ScopeIDMap[scopeid]->AddResponseMap(mapname, themap.release());
	}

	ExpectInstruction(Bytecode::Futures);

	UINT_PTR numfutures = ReadNumber();
	for(UINT_PTR i = 0; i < numfutures; ++i)
	{
		const std::wstring& futurename = WidenAndCache(ReadNullTerminatedString());

		VM::Block* tempblock = new VM::Block;
		GenerateOpFromByteCode(ReadInstruction(), tempblock);
		if(!IsPrepass)
			ScopeIDMap[scopeid]->AddFuture(futurename, VM::OperationPtr(tempblock->PopTailOperation()));
		delete tempblock;
	}

	ExpectInstruction(Bytecode::ListTypes);

	UINT_PTR numlisttypes = ReadNumber();
	for(UINT_PTR i = 0; i < numlisttypes; ++i)
	{
		const std::wstring& listname = WidenAndCache(ReadNullTerminatedString());
		Integer32 listtype = ReadNumber();
		if(!IsPrepass)
			ScopeIDMap[scopeid]->SetListType(listname, static_cast<VM::EpochVariableTypeID>(listtype));
	}


	ExpectInstruction(Bytecode::ListSizes);

	UINT_PTR numlistsizes = ReadNumber();
	for(UINT_PTR i = 0; i < numlistsizes; ++i)
	{
		const std::wstring& listname = WidenAndCache(ReadNullTerminatedString());
		Integer32 listsize = ReadNumber();
		if(!IsPrepass)
			ScopeIDMap[scopeid]->SetListSize(listname, listsize);
	}

	ExpectInstruction(Bytecode::EndScope);

	return ScopeIDMap[scopeid];
}

//
// Load a function signature
//
VM::FunctionSignature FileLoader::LoadFunctionSignature()
{
	Integer32 numparams = ReadNumber();
	std::vector<VM::EpochVariableTypeID> paramtypes;
	for(Integer32 j = 0; j < numparams; ++j)
		paramtypes.push_back(static_cast<VM::EpochVariableTypeID>(ReadNumber()));

	Integer32 numreturns = ReadNumber();
	std::vector<VM::EpochVariableTypeID> returntypes;
	for(Integer32 j = 0; j < numreturns; ++j)
		returntypes.push_back(static_cast<VM::EpochVariableTypeID>(ReadNumber()));

	Integer32 numparamhints = ReadNumber();
	std::vector<IDType> paramhints;
	for(Integer32 j = 0; j < numparamhints; ++j)
		paramhints.push_back(ReadNumber());

	Integer32 numparamflags = ReadNumber();
	std::vector<UInteger32> paramflags;
	for(Integer32 j = 0; j < numparamflags; ++j)
		paramflags.push_back(ReadNumber());

	Integer32 numsubsignatures = ReadNumber();
	std::vector<VM::FunctionSignature*> subsignatures;
	for(Integer32 j = 0; j < numsubsignatures; ++j)
	{
		unsigned char instruction = ReadInstruction();
		if(instruction == Bytecode::FunctionSignatureEnd)
		{
			if(!IsPrepass)
				subsignatures.push_back(NULL);
		}
		else
		{
			std::auto_ptr<VM::FunctionSignature> subsignature(new VM::FunctionSignature(LoadFunctionSignature()));
			if(!IsPrepass)
				subsignatures.push_back(subsignature.release());
		}
	}

	Integer32 numreturnhints = ReadNumber();
	std::vector<IDType> returnhints;
	for(Integer32 j = 0; j < numreturnhints; ++j)
		returnhints.push_back(ReadNumber());

	ExpectInstruction(Bytecode::FunctionSignatureEnd);

	if(!IsPrepass)
	{
		VM::FunctionSignature signature;
		for(size_t j = 0; j < paramtypes.size(); ++j)
		{
			signature.AddParam(paramtypes[j], paramhints[j], subsignatures[j]);
			if(paramflags[j] & VM::FunctionSignature::PARAMTYPEFLAG_ISREFERENCE)
				signature.SetLastParamToReference();
		}
		for(size_t j = 0; j < returntypes.size(); ++j)
			signature.AddReturn(returntypes[j], returnhints[j]);

		return signature;
	}

	return VM::FunctionSignature();
}

//
// Load code operations, stopping when the block is exited
//
VM::Block* FileLoader::LoadCodeBlock()
{
	std::auto_ptr<VM::Block> newblock(NULL);
	if(!IsPrepass)
		newblock.reset(new VM::Block);

	while(true)
	{
		unsigned char instruction = ReadInstruction();
		if(instruction == Bytecode::EndBlock)
			return newblock.release();

		GenerateOpFromByteCode(instruction, newblock.get());
	}
}


//
// Turn the bytecode into a VM op
//
void FileLoader::GenerateOpFromByteCode(unsigned char instruction, VM::Block* newblock)
{
	if(instruction == Bytecode::PushOperation)
	{
		unsigned char op = ReadInstruction();
		GenerateOpFromByteCode(op, newblock);
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::PushOperation(newblock->PopTailOperation().release())));
	}
	else if(instruction == Bytecode::Invoke)
	{
		FunctionID funcid = ReadNumber();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::Invoke(FunctionIDMap.find(funcid)->second, false)));
	}
	else if(instruction == Bytecode::DebugWrite)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::DebugWriteStringExpression));
	}
	else if(instruction == Bytecode::PushRealLiteral)
	{
		Real value = ReadFloat();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::PushRealLiteral(value)));
	}
	else if(instruction == Bytecode::DivideReals)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		UInteger32 paramcount = ReadNumber();
		if(!IsPrepass)
		{
			if(paramcount == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::DivideReals));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::DivideReals(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::PushIntegerLiteral)
	{
		Integer32 value = ReadNumber();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(value)));
	}
	else if(instruction == Bytecode::IsEqual)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::IsEqual(type)));
	}
	else if(instruction == Bytecode::IsNotEqual)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::IsNotEqual(type)));
	}
	else if(instruction == Bytecode::IsLesser)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::IsLesser(type)));
	}
	else if(instruction == Bytecode::IsGreater)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::IsGreater(type)));
	}
	else if(instruction == Bytecode::AssignValue)
	{
		std::string varname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::AssignValue(WidenAndCache(varname))));
	}
	else if(instruction == Bytecode::DoWhile)
	{
		ExpectInstruction(Bytecode::BeginBlock);
		VM::ScopeDescription* scope = LoadScope(false);
		std::auto_ptr<VM::Block> theblock(LoadCodeBlock());
		if(!IsPrepass)
		{
			theblock->BindToScope(UnregisterScopeToDelete(scope));
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::DoWhileLoop(theblock.release())));
		}
	}
	else if(instruction == Bytecode::GetValue)
	{
		std::string varname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::GetVariableValue(WidenAndCache(varname))));
	}
	else if(instruction == Bytecode::If)
	{
		std::auto_ptr<VM::Block> trueblock(NULL);
		std::auto_ptr<VM::Block> falseblock(NULL);

		unsigned char nextop = ReadInstruction();
		if(nextop == Bytecode::BeginBlock)
		{
			VM::ScopeDescription* scope = LoadScope(false);
			trueblock.reset(LoadCodeBlock());
			if(!IsPrepass)
				trueblock->BindToScope(UnregisterScopeToDelete(scope));
		}

		std::auto_ptr<VM::Operations::If> ifop(NULL);
		if(!IsPrepass)
			ifop.reset(new VM::Operations::If(trueblock.release(), NULL));

		nextop = ReadInstruction();
		if(nextop == Bytecode::ElseIfWrapper)
		{
			do
			{
				nextop = ReadInstruction();
				if(nextop == Bytecode::ElseIf)
					nextop = ReadInstruction();

				if(nextop == Bytecode::BeginBlock)
				{
					VM::ScopeDescription* scope = LoadScope(false);
					std::auto_ptr<VM::Block> elseifwrapblock(LoadCodeBlock());
					if(!IsPrepass)
					{
						elseifwrapblock->BindToScope(UnregisterScopeToDelete(scope));
						ifop->SetElseIfBlock(new VM::Operations::ElseIfWrapper(elseifwrapblock.release()));
					}				
				}
				else
					throw InvalidBytecodeException("Elseifwrap instruction loaded, but no elseif blocks found! This is probably a compiler bug.");
			} while(PeekInstruction() == Bytecode::ElseIf);
		}

		nextop = ReadInstruction();
		if(nextop == Bytecode::BeginBlock)
		{
			VM::ScopeDescription* scope = LoadScope(false);
			falseblock.reset(LoadCodeBlock());
			if(!IsPrepass)
			{
				falseblock->BindToScope(UnregisterScopeToDelete(scope));
				ifop->SetFalseBlock(falseblock.release());
			}
		}

		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(ifop.release()));
	}
	else if(instruction == Bytecode::AddReals)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		UInteger32 paramcount = ReadNumber();
		if(!IsPrepass)
		{
			if(paramcount == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SumReals));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SumReals(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::SubReals)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		UInteger32 paramcount = ReadNumber();
		if(!IsPrepass)
		{
			if(paramcount == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SubtractReals));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SubtractReals(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::PushBooleanLiteral)
	{
		bool value = ReadFlag();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::PushBooleanLiteral(value)));
	}
	else if(instruction == Bytecode::PushStringLiteral)
	{
		Integer32 len = ReadNumber();
		std::string str = ReadStringByLength(len);
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::PushStringLiteral(WidenAndCache(str))));
	}
	else if(instruction == Bytecode::AddIntegers)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		UInteger32 paramcount = ReadNumber();
		if(!IsPrepass)
		{
			if(paramcount == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SumIntegers));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SumIntegers(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::SubtractIntegers)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		UInteger32 paramcount = ReadNumber();
		if(!IsPrepass)
		{
			if(paramcount == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SubtractIntegers));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::SubtractIntegers(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::DebugRead)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::DebugReadStaticString));
	}
	else if(instruction == Bytecode::ElseIf)
	{
		if(ReadInstruction() != Bytecode::BeginBlock)
			throw InvalidBytecodeException("Corruption near Elseif instruction (expected to begin a block here)");

		VM::ScopeDescription* scope = LoadScope(false);
		std::auto_ptr<VM::Block> block(LoadCodeBlock());

		if(!IsPrepass)
		{
			block->BindToScope(UnregisterScopeToDelete(scope));
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ElseIf(block.release())));
		}
	}
	else if(instruction == Bytecode::ExitIfChain)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ExitIfChain));
	}
	else if(instruction == Bytecode::ReadTuple)
	{
		std::string varname = ReadNullTerminatedString();
		std::string membername = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ReadTuple(WidenAndCache(varname), WidenAndCache(membername))));
	}
	else if(instruction == Bytecode::WriteTuple)
	{
		std::string varname = ReadNullTerminatedString();
		std::string membername = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::AssignTuple(WidenAndCache(varname), WidenAndCache(membername))));
	}
	else if(instruction == Bytecode::ReadStructure)
	{
		std::string varname = ReadNullTerminatedString();
		std::string membername = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ReadStructure(WidenAndCache(varname), WidenAndCache(membername))));
	}
	else if(instruction == Bytecode::WriteStructure)
	{
		std::string varname = ReadNullTerminatedString();
		std::string membername = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::AssignStructure(WidenAndCache(varname), WidenAndCache(membername))));
	}
	else if(instruction == Bytecode::Init)
	{
		std::string varname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::InitializeValue(WidenAndCache(varname))));
	}
	else if(instruction == Bytecode::BindFunctionReference)
	{
		std::string funcname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::BindFunctionReference(WidenAndCache(funcname))));
	}
	else if(instruction == Bytecode::SizeOf)
	{
		std::string varname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::SizeOf(WidenAndCache(varname))));
	}
	else if(instruction == Bytecode::While)
	{
		ExpectInstruction(Bytecode::BeginBlock);
		VM::ScopeDescription* scope = LoadScope(false);
		std::auto_ptr<VM::Block> loopblock(LoadCodeBlock());
		if(!IsPrepass)
		{
			loopblock->BindToScope(UnregisterScopeToDelete(scope));
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::WhileLoop(loopblock.release())));
		}
	}
	else if(instruction == Bytecode::BindReference)
	{
		std::string varname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::BindReference(WidenAndCache(varname))));
	}
	else if(instruction == Bytecode::WhileCondition)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::WhileLoopConditional));
	}
	else if(instruction == Bytecode::Break)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::Break));
	}
	else if(instruction == Bytecode::Return)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::Return));
	}
	else if(instruction == Bytecode::BitwiseAnd)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());

		std::auto_ptr<VM::Operations::BitwiseAnd> op(NULL);
		if(!IsPrepass)
			op.reset(new VM::Operations::BitwiseAnd(type));

		UInteger32 testcount = ReadNumber();
		for(UInteger32 i = 0; i < testcount; ++i)
		{
			std::auto_ptr<VM::Block> tempblock(new VM::Block(false));
			GenerateOpFromByteCode(ReadInstruction(), tempblock.get());

			if(!IsPrepass)
				op->AddOperation(tempblock->PopTailOperation().release());
		}

		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(op.release()));
	}
	else if(instruction == Bytecode::BitwiseOr)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());

		std::auto_ptr<VM::Operations::BitwiseOr> op(NULL);
		if(!IsPrepass)
			op.reset(new VM::Operations::BitwiseOr(type));

		UInteger32 testcount = ReadNumber();
		for(UInteger32 i = 0; i < testcount; ++i)
		{
			std::auto_ptr<VM::Block> tempblock(new VM::Block(false));
			GenerateOpFromByteCode(ReadInstruction(), tempblock.get());

			if(!IsPrepass)
				op->AddOperation(tempblock->PopTailOperation().release());
		}

		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(op.release()));
	}
	else if(instruction == Bytecode::BitwiseXor)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());

		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::BitwiseXor(type)));
	}
	else if(instruction == Bytecode::BitwiseNot)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());

		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::BitwiseNot(type)));
	}
	else if(instruction == Bytecode::LogicalAnd)
	{
		std::auto_ptr<VM::Operations::LogicalAnd> op(NULL);
		if(!IsPrepass)
			op.reset(new VM::Operations::LogicalAnd);

		UInteger32 testcount = ReadNumber();
		for(UInteger32 i = 0; i < testcount; ++i)
		{
			std::auto_ptr<VM::Block> tempblock(new VM::Block(false));
			GenerateOpFromByteCode(ReadInstruction(), tempblock.get());

			if(!IsPrepass)
				op->AddOperation(tempblock->PopTailOperation().release());
		}

		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(op.release()));
	}
	else if(instruction == Bytecode::LogicalOr)
	{
		std::auto_ptr<VM::Operations::LogicalOr> op;
		if(!IsPrepass)
			op.reset(new VM::Operations::LogicalOr);

		UInteger32 testcount = ReadNumber();
		for(UInteger32 i = 0; i < testcount; ++i)
		{
			std::auto_ptr<VM::Block> tempblock(new VM::Block(false));
			GenerateOpFromByteCode(ReadInstruction(), tempblock.get());

			if(!IsPrepass)
				op->AddOperation(tempblock->PopTailOperation().release());
		}

		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(op.release()));
	}
	else if(instruction == Bytecode::LogicalXor)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::LogicalXor));
	}
	else if(instruction == Bytecode::LogicalNot)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::LogicalNot));
	}
	else if(instruction == Bytecode::Concat)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		UInteger32 paramcount = ReadNumber();
		if(!IsPrepass)
		{
			if(paramcount == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::Concatenate));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::Concatenate(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::IsGreaterEqual)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::IsGreaterOrEqual(type)));
	}
	else if(instruction == Bytecode::PushInteger16Literal)
	{
		Integer16 value = static_cast<Integer16>(ReadNumber());
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::PushInteger16Literal(value)));
	}
	else if(instruction == Bytecode::InvokeIndirect)
	{
		std::string funcname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::InvokeIndirect(WidenAndCache(funcname))));
	}
	else if(instruction == Bytecode::BooleanLiteral)
	{
		bool flag = ReadFlag();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::BooleanConstant(flag)));
	}
	else if(instruction == Bytecode::BeginBlock)
	{
		VM::ScopeDescription* scope = LoadScope(false);
		std::auto_ptr<VM::Block> theblock(LoadCodeBlock());
		if(!IsPrepass)
		{
			theblock->BindToScope(UnregisterScopeToDelete(scope));
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ExecuteBlock(theblock.release())));
		}
	}
	else if(instruction == Bytecode::ConsList)
	{
		VM::EpochVariableTypeID type = static_cast<VM::EpochVariableTypeID>(ReadNumber());
		size_t numentries = ReadNumber();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ConsList(numentries, type)));
	}
	else if(instruction == Bytecode::ReadStructureIndirect)
	{
		std::string membername = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ReadStructureIndirect(WidenAndCache(membername), newblock->GetTailOperation())));
	}
	else if(instruction == Bytecode::BindStruct)
	{
		std::string membername, varname;

		bool chained = ReadFlag();
		if(!chained)
			varname = ReadNullTerminatedString();

		membername = ReadNullTerminatedString();

		if(!IsPrepass)
		{
			if(chained)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::BindStructMemberReference(WidenAndCache(membername))));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::BindStructMemberReference(WidenAndCache(varname), WidenAndCache(membername))));
		}
	}
	else if(instruction == Bytecode::WriteStructureIndirect)
	{
		std::string membername = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::AssignStructureIndirect(WidenAndCache(membername))));
	}
	else if(instruction == Bytecode::ForkTask)
	{
		ExpectInstruction(Bytecode::BeginBlock);
		VM::ScopeDescription* scope = LoadScope(false);
		std::auto_ptr<VM::Block> taskblock(LoadCodeBlock());
		if(!IsPrepass)
		{
			scope->ParentScope = &LoadingProgram->GetGlobalScope();
			taskblock->BindToScope(UnregisterScopeToDelete(scope));
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ForkTask(taskblock.release())));
		}
	}
	else if(instruction == Bytecode::AcceptMessage)
	{
		std::string messagename = ReadNullTerminatedString();

		UINT_PTR numparams = ReadNumber();
		std::vector<VM::EpochVariableTypeID> paramtypes;
		for(UINT_PTR i = 0; i < numparams; ++i)
			paramtypes.push_back(static_cast<VM::EpochVariableTypeID>(ReadNumber()));

		ExpectInstruction(Bytecode::BeginBlock);
		VM::ScopeDescription* responsescope = LoadScope(false);
		std::auto_ptr<VM::Block> responseblock(LoadCodeBlock());
		VM::ScopeDescription* auxscope = LoadScope(false);
		if(!IsPrepass)
		{
			responseblock->BindToScope(UnregisterScopeToDelete(responsescope));
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::AcceptMessage(WidenAndCache(messagename), responseblock.release(), UnregisterScopeToDelete(auxscope))));
		}
	}
	else if(instruction == Bytecode::MultiplyIntegers)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		UInteger32 paramcount = ReadNumber();
		if(!IsPrepass)
		{
			if(paramcount == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::MultiplyIntegers));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::MultiplyIntegers(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::GetMessageSender)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::GetMessageSender));
	}
	else if(instruction == Bytecode::GetTaskCaller)
	{
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::GetTaskCaller));
	}
	else if(instruction == Bytecode::SendTaskMessage)
	{
		std::string targettaskname;
		bool targettaskbyname = ReadFlag();
		std::string messagename = ReadNullTerminatedString();
		UINT_PTR numparams = ReadNumber();
		std::list<VM::EpochVariableTypeID> paramtypes;
		for(UINT_PTR i = 0; i < numparams; ++i)
			paramtypes.push_back(static_cast<VM::EpochVariableTypeID>(ReadNumber()));
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::SendTaskMessage(targettaskbyname, WidenAndCache(messagename), paramtypes)));
	}
	else if(instruction == Bytecode::AcceptMessageFromMap)
	{
		std::string mapname = ReadNullTerminatedString();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::AcceptMessageFromResponseMap(WidenAndCache(mapname))));
	}
	else if(instruction == Bytecode::TypeCastToString)
	{
		Integer32 originaltype = ReadNumber();
		if(!IsPrepass)
		{
			switch(originaltype)
			{
			case VM::EpochVariableType_Real:
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCastToString<TypeInfo::RealT>));
				break;
			case VM::EpochVariableType_Integer:
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCastToString<TypeInfo::IntegerT>));
				break;
			case VM::EpochVariableType_Integer16:
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCastToString<TypeInfo::Integer16T>));
				break;
			case VM::EpochVariableType_Boolean:
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCastBooleanToString()));
				break;
			case VM::EpochVariableType_Buffer:
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCastBufferToString()));
				break;
			default:
				throw Exception("Cannot cast the given variable type to string; is one or more of your libraries out of date?");
			}
		}
	}
	else if(instruction == Bytecode::DivideIntegers)
	{
		bool firstislist = ReadFlag();
		bool secondislist = ReadFlag();
		Integer32 numparams = ReadNumber();
		if(!IsPrepass)
		{
			if(numparams == 1)
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::DivideIntegers));
			else
				newblock->AddOperation(VM::OperationPtr(new VM::Operations::DivideIntegers(firstislist, secondislist)));
		}
	}
	else if(instruction == Bytecode::TypeCast)
	{
		Integer32 origintype = ReadNumber();
		Integer32 desttype = ReadNumber();
		if(!IsPrepass)
		{
			if(desttype == VM::EpochVariableType_Integer)
			{
				switch(origintype)
				{
				case VM::EpochVariableType_String:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::IntegerT>));
					break;
				case VM::EpochVariableType_Real:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::RealT, TypeInfo::IntegerT>));
					break;
				case VM::EpochVariableType_Integer16:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::Integer16T, TypeInfo::IntegerT>));
					break;
				case VM::EpochVariableType_Boolean:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::IntegerT>));
					break;
				default:
					throw Exception("Invalid parameters supplied to typecast operation; ensure all libraries are up to date and the binary is not corrupted");
				}
			}
			else if(desttype == VM::EpochVariableType_Integer16)
			{
				switch(origintype)
				{
				case VM::EpochVariableType_String:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::Integer16T>));
					break;
				case VM::EpochVariableType_Real:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::RealT, TypeInfo::Integer16T>));
					break;
				case VM::EpochVariableType_Integer:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::IntegerT, TypeInfo::Integer16T>));
					break;
				case VM::EpochVariableType_Boolean:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::Integer16T>));
					break;
				default:
					throw Exception("Invalid parameters supplied to typecast operation; ensure all libraries are up to date and the binary is not corrupted");
				}
			}
			else if(desttype == VM::EpochVariableType_Real)
			{
				switch(origintype)
				{
				case VM::EpochVariableType_String:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::RealT>));
					break;
				case VM::EpochVariableType_Integer:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::IntegerT, TypeInfo::RealT>));
					break;
				case VM::EpochVariableType_Integer16:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::Integer16T, TypeInfo::RealT>));
					break;
				case VM::EpochVariableType_Boolean:
					newblock->AddOperation(VM::OperationPtr(new VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::RealT>));
					break;
				default:
					throw Exception("Invalid parameters supplied to typecast operation; ensure all libraries are up to date and the binary is not corrupted");
				}
			}
			else
				throw Exception("Invalid parameters supplied to typecast operation; ensure all libraries are up to date and the binary is not corrupted");
		}
	}
	else if(instruction == Bytecode::Future)
	{
		const std::wstring& futurename = WidenAndCache(ReadNullTerminatedString());
		Integer32 type = ReadNumber();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ForkFuture(futurename, static_cast<VM::EpochVariableTypeID>(type))));
	}
	else if(instruction == Bytecode::Map)
	{
		VM::Block* tempblock = new VM::Block;
		GenerateOpFromByteCode(ReadInstruction(), tempblock);
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::MapOperation(tempblock->PopTailOperation())));
		delete tempblock;
	}
	else if(instruction == Bytecode::Reduce)
	{
		VM::Block* tempblock = new VM::Block;
		GenerateOpFromByteCode(ReadInstruction(), tempblock);
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::ReduceOperation(tempblock->PopTailOperation())));
		delete tempblock;
	}
	else if(instruction == Bytecode::IsLesserEqual)
	{
		Integer32 type = ReadNumber();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::IsLesserOrEqual(static_cast<VM::EpochVariableTypeID>(type))));
	}
	else if(instruction == Bytecode::IntegerLiteral)
	{
		Integer32 value = ReadNumber();
		if(!IsPrepass)
			newblock->AddOperation(VM::OperationPtr(new VM::Operations::IntegerConstant(value)));
	}
	else
		throw InvalidBytecodeException("Read an opcode from the binary, but it doesn't match any known opcode. Aborting program execution!");
}

//
// Load the special block that initializes global variables
//
void FileLoader::LoadGlobalInitBlock()
{
	ExpectInstruction(Bytecode::GlobalBlock);
	unsigned char instruction = ReadInstruction();
	if(instruction == Bytecode::BeginBlock)
	{
		std::auto_ptr<VM::Block> initblock(LoadCodeBlock());
		if(!IsPrepass)
		{
			initblock->DoNotDeleteScope();
			LoadingProgram->ReplaceGlobalInitBlock(initblock.release());
		}
	}
}

//
// Helper for widening and caching strings
//
const std::wstring& FileLoader::WidenAndCache(const std::string& str)
{
	return LoadingProgram->PoolStaticString(widen(str));
}


//
// Helpers for tracking scopes that need to be freed in case of self-destruction
//
VM::ScopeDescription* FileLoader::RegisterScopeToDelete(VM::ScopeDescription* scope)
{
	DeleteScopes.insert(scope);
	return scope;
}

VM::ScopeDescription* FileLoader::UnregisterScopeToDelete(VM::ScopeDescription* scope)
{
	std::set<VM::ScopeDescription*>::iterator iter = DeleteScopes.find(scope);
	if(iter != DeleteScopes.end())
		DeleteScopes.erase(iter);

	return scope;
}
