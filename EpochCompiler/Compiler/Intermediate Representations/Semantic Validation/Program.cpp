//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a complete Epoch program
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"

#include "Compiler/Session.h"
#include "Compiler/Exceptions.h"

#include "Utility/StringPool.h"

#include <sstream>


using namespace IRSemantics;


//
// Construct and initialize a program
//
Program::Program(StringPoolManager& strings, CompileSession& session)
	: CounterAnonParam(0),
	  CounterLexicalScope(0),
	  Strings(strings),
	  StructureTypeCounter(VM::EpochType_CustomBase),
	  Session(session),
	  GlobalScope(NULL)
{
}

//
// Destruct and clean up a program
//
Program::~Program()
{
	for(boost::unordered_map<StringHandle, Function*>::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
		delete iter->second;

	for(std::map<StringHandle, Structure*>::iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
		delete iter->second;

	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
		delete *iter;

	delete GlobalScope;
}


//
// Add a string to the program's string pool
//
StringHandle Program::AddString(const std::wstring& str)
{
	StringHandle ret = IdentifierCache.Find(str);
	if(!ret)
	{
		ret = Strings.Pool(str);
		IdentifierCache.Add(str, ret);
	}
	
	return ret;
}

const std::wstring& Program::GetString(StringHandle handle) const
{
	return Strings.GetPooledString(handle);
}


//
// Add a structure definition to the program
//
void Program::AddStructure(StringHandle name, Structure* structure)
{
	if(Structures.find(name) != Structures.end())
	{
		delete structure;
		throw std::runtime_error("Duplicate structure name");		// TODO - this should not be an exception
	}

	Structures.insert(std::make_pair(name, structure));
	StructureTypes.insert(std::make_pair(name, ++StructureTypeCounter));

	const std::wstring& structurename = GetString(name);
	const std::vector<std::pair<StringHandle, StructureMember*> >& members = structure->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
	{
		std::wstring overloadname = GenerateStructureMemberAccessOverloadName(structurename, GetString(iter->first));
		StructureMemberAccessOverloadNameCache.Add(std::make_pair(name, iter->first), Strings.PoolFast(overloadname));
	}
}


//
// Allocate a function overload mangled name
//
StringHandle Program::CreateFunctionOverload(const std::wstring& name)
{
	StringHandle handle = AddString(name);
	unsigned counter = FunctionOverloadCounters[handle]++;
	StringHandle ret = AddString(GenerateFunctionOverloadName(handle, counter));
	FunctionOverloadNameCache.Add(std::make_pair(handle, counter), ret);
	return ret;
}

std::wstring Program::GenerateFunctionOverloadName(StringHandle name, size_t index) const
{
	if(index == 0)
		return GetString(name);

	std::wostringstream format;
	format << GetString(name) << L"@@overload@" << index;

	return format.str();
}


//
// Add a function definition to the program
//
void Program::AddFunction(StringHandle name, Function* function)
{
	if(Functions.find(name) != Functions.end())
	{
		delete function;
		throw std::runtime_error("Duplicate function name");		// TODO - this should not be an exception
	}

	Functions.insert(std::make_pair(name, function));
}

//
// Determine if the program contains a function by the given name
//
bool Program::HasFunction(StringHandle name) const
{
	return (Functions.find(name) != Functions.end());
}

//
// Add a global code block to a program
//
size_t Program::AddGlobalCodeBlock(CodeBlock* code)
{
	size_t index = GlobalCodeBlocks.size();
	AddString(GenerateAnonymousGlobalScopeName(index));
	GlobalCodeBlocks.push_back(code);
	AddScope(code->GetScope());
	return index;
}

//
// Retrieve a given global code block from the program
//
const CodeBlock& Program::GetGlobalCodeBlock(size_t index) const
{
	if(index >= GlobalCodeBlocks.size())
	{
		//
		// This is a failure on the part of the caller.
		//
		// Callers should pay attention to how many global code blocks
		// have been defined and make sure they don't ask for one that
		// doesn't exist. See GetNumGlobalCodeBlocks().
		//
		throw InternalException("Requested a global code block index that has not been defined");
	}

	return *GlobalCodeBlocks[index];
}

//
// Retrieve the count of global code blocks in a given program
//
size_t Program::GetNumGlobalCodeBlocks() const
{
	return GlobalCodeBlocks.size();
}

//
// Retrieve the name of a given global code block
//
StringHandle Program::GetGlobalCodeBlockName(size_t index) const
{
	return Strings.Find(GenerateAnonymousGlobalScopeName(index));
}

//
// Validate a program
//
bool Program::Validate() const
{
	bool valid = true;

	for(std::map<StringHandle, Structure*>::const_iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
	{
		if(!iter->second->Validate(*this))
			valid = false;
	}

	for(std::vector<CodeBlock*>::const_iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->Validate(*this))
			valid = false;
	}

	for(boost::unordered_map<StringHandle, Function*>::const_iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(!iter->second->Validate(*this))
			valid = false;
	}

	return valid;
}

bool Program::TypeInference()
{
	InferenceContext context(0, InferenceContext::CONTEXT_GLOBAL);

	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->TypeInference(*this, context))
			return false;
	}

	for(boost::unordered_map<StringHandle, Function*>::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(!iter->second->TypeInference(*this, context))
			return false;
	}

	return true;
}

bool Program::CompileTimeCodeExecution()
{
	for(std::map<StringHandle, Structure*>::iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
	{
		if(!iter->second->CompileTimeCodeExecution(iter->first, *this))
			return false;
	}

	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(*this))
			return false;
	}

	for(boost::unordered_map<StringHandle, Function*>::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
	{
		if(!iter->second->CompileTimeCodeExecution(*this))
			return false;
	}

	return true;
}


//
// Allocate a name for an anonymous function parameter (usually an expression)
//
StringHandle Program::AllocateAnonymousParamName()
{
	std::wostringstream name;
	name << L"@@anonparam@" << (++CounterAnonParam);
	return Strings.PoolFast(name.str());
}

std::wstring Program::GenerateAnonymousGlobalScopeName(size_t index)
{
	std::wostringstream name;
	name << L"@@globalscope@" << index;
	return name.str();
}


StringHandle Program::AllocateLexicalScopeName(const CodeBlock* blockptr)
{
	StringHandle ret = Strings.PoolFast(GenerateLexicalScopeName(blockptr->GetScope()));
	LexicalScopeNameCache.Add(blockptr->GetScope(), ret);
	return ret;
}

StringHandle Program::FindLexicalScopeName(const CodeBlock* blockptr) const
{
	return LexicalScopeNameCache.Find(blockptr->GetScope());
}

StringHandle Program::FindLexicalScopeName(const ScopeDescription* scopeptr) const
{
	return LexicalScopeNameCache.Find(scopeptr);
}

std::wstring Program::GenerateLexicalScopeName(const ScopeDescription* scopeptr)
{
	std::wostringstream name;
	name << L"@@scope@" << scopeptr;
	return name.str();
}


VM::EpochTypeID Program::LookupType(StringHandle name) const
{
	// TODO - replace this with a less hard-coded solution
	const std::wstring& type = Strings.GetPooledString(name);

	if(type == L"integer")
		return VM::EpochType_Integer;
	else if(type == L"integer16")
		return VM::EpochType_Integer16;
	else if(type == L"string")
		return VM::EpochType_String;
	else if(type == L"boolean")
		return VM::EpochType_Boolean;
	else if(type == L"real")
		return VM::EpochType_Real;
	else if(type == L"buffer")
		return VM::EpochType_Buffer;
	else if(type == L"identifier")
		return VM::EpochType_Identifier;

	std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = StructureTypes.find(name);
	if(iter != StructureTypes.end())
		return iter->second;

	return VM::EpochType_Error;
}


VM::EpochTypeID Program::GetStructureMemberType(StringHandle structurename, StringHandle membername) const
{
	std::map<StringHandle, Structure*>::const_iterator iter = Structures.find(structurename);
	if(iter == Structures.end())
	{
		//
		// This should occur only when something has gone horribly
		// wrong in the parsing system, i.e. there is a logic bug
		// in the compiler implementation itself.
		//
		// Essentially this means someone thought they had a token
		// that was bound to a structure type definition, but either
		// it never was, or we forgot the type definition metadata
		// somehow during compilation.
		//
		throw InternalException("String handle does not correspond to any known structure type");
	}

	const std::vector<std::pair<StringHandle, StructureMember*> >& members = iter->second->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
	{
		if(memberiter->first == membername)
			return memberiter->second->GetEpochType(*this);
	}
	
	//
	// This is also limited to logic bugs in the compiler.
	//
	// We should not attempt to access a structure member's type
	// if the structure has no members by that name. Semantic
	// checking should have first determined that the member does
	// not exist, prior to ever asking for the type information.
	//
	// Further, semantic checking should have failed once the
	// missing member was detected, preventing any further type
	// validation surrounding the faulty expression.
	//
	throw InternalException("Structure type does not provide any members by this name");
}

StringHandle Program::GetNameOfStructureType(VM::EpochTypeID structuretype) const
{
	for(std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = StructureTypes.begin(); iter != StructureTypes.end(); ++iter)
	{
		if(iter->second == structuretype)
			return iter->first;
	}

	//
	// This catches a potential logic bug in the compiler implementation.
	//
	// If a structure has been allocated a type ID, that type ID should never
	// be able to exist without being also bound to an identifier, even if that
	// identifier is just an automatically generated magic anonymous token.
	//
	// Callers cannot be faulted directly for this exception; the problem most
	// likely lies elsewhere in the structure type handling code.
	//
	throw InternalException("Type ID is not bound to any known identifier");
}

StringHandle Program::FindStructureMemberAccessOverload(StringHandle structurename, StringHandle membername) const
{
	return StructureMemberAccessOverloadNameCache.Find(std::make_pair(structurename, membername));
}

std::wstring Program::GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername)
{
	return L".@@" + structurename + L"@@" + membername;
}


Bytecode::EntityTag Program::GetEntityTag(StringHandle identifier) const
{
	for(EntityTable::const_iterator iter = Session.InfoTable.Entities->begin(); iter != Session.InfoTable.Entities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = Session.InfoTable.ChainedEntities->begin(); iter != Session.InfoTable.ChainedEntities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = Session.InfoTable.PostfixEntities->begin(); iter != Session.InfoTable.PostfixEntities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	//
	// This represents a failure in the parser.
	//
	// The parser should reject any code which attempts to utilize an undefined
	// entity. By the time we reach semantic validation, it should no longer be
	// possible to refer to an entity identifier which doesn't have any backing
	// descriptor data (including the entity type-tag we're retrieving here).
	//
	throw InternalException("String handle does not correspond to any known type of entity");
}

Bytecode::EntityTag Program::GetEntityCloserTag(StringHandle identifier) const
{
	for(EntityTable::const_iterator iter = Session.InfoTable.PostfixClosers->begin(); iter != Session.InfoTable.PostfixClosers->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	//
	// This represents a failure in the parser.
	//
	// The parser should reject any code which attempts to utilize an undefined
	// entity. By the time we reach semantic validation, it should no longer be
	// possible to refer to an entity identifier which doesn't have any backing
	// descriptor data (including the entity type-tag we're retrieving here).
	//
	throw InternalException("String handle does not correspond to any known type of entity closer");
}


ScopeDescription* Program::GetGlobalScope()
{
	if(!GlobalScope)
		GlobalScope = new ScopeDescription;

	return GlobalScope;
}


InferenceContext::PossibleParameterTypes Program::GetExpectedTypesForStatement(StringHandle name) const
{
	boost::unordered_map<StringHandle, unsigned>::const_iterator iter = FunctionOverloadCounters.find(name);
	if(iter != FunctionOverloadCounters.end())
	{
		InferenceContext::PossibleParameterTypes ret;
		ret.reserve(iter->second);

		for(size_t i = 0; i < iter->second; ++i)
		{
			StringHandle overloadname = i ? FunctionOverloadNameCache.Find(std::make_pair(name, i)) : name;
			boost::unordered_map<StringHandle, Function*>::const_iterator funciter = Functions.find(overloadname);
			if(funciter == Functions.end())
			{
				//
				// This is a critical internal failure. A function overload name
				// has been registered but the corresponding definition of the
				// function cannot be located.
				//
				// Examine the handling of overload registration, name resolution,
				// and definition storage. We should not reach the phase of type
				// inference until all function definitions have been visited by
				// AST traversal in prior semantic validation passes.
				//
				throw InternalException("Function overload registered but definition not found");
			}

			ret.push_back(InferenceContext::TypePossibilities());

			std::vector<StringHandle> paramnames = funciter->second->GetParameterNames();
			for(std::vector<StringHandle>::const_iterator paramiter = paramnames.begin(); paramiter != paramnames.end(); ++paramiter)
				ret.back().push_back(funciter->second->GetParameterType(*paramiter, *this));
		}

		return ret;
	}

	OverloadMap::const_iterator ovmapiter = Session.FunctionOverloadNames.find(name);
	if(ovmapiter != Session.FunctionOverloadNames.end())
	{
		InferenceContext::PossibleParameterTypes ret(ovmapiter->second.size(), InferenceContext::TypePossibilities());
		size_t i = 0;
		for(StringHandleSet::const_iterator oviter = ovmapiter->second.begin(); oviter != ovmapiter->second.end(); ++oviter)
		{
			FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(*oviter);
			if(fsigiter != Session.FunctionSignatures.end())
			{
				for(size_t j = 0; j < fsigiter->second.GetNumParameters(); ++j)
					ret[i].push_back(fsigiter->second.GetParameter(j).Type);
			}

			++i;
		}
		
		return ret;
	}

	FunctionSignatureSet::const_iterator fsigiter = Session.FunctionSignatures.find(name);
	if(fsigiter != Session.FunctionSignatures.end())
	{
		InferenceContext::PossibleParameterTypes ret(1, InferenceContext::TypePossibilities());
		for(size_t i = 0; i < fsigiter->second.GetNumParameters(); ++i)
			ret.front().push_back(fsigiter->second.GetParameter(i).Type);

		return ret;
	}

	// TODO - this should not be an exception. Record a semantic error instead.
	throw std::runtime_error("Invalid function name");
}

void Program::AddScope(ScopeDescription* scope)
{
	AddScope(scope, LexicalScopeNameCache.Find(scope));
}

void Program::AddScope(ScopeDescription* scope, StringHandle name)
{
	LexicalScopes.insert(std::make_pair(name, scope));
}

