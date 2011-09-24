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

#include "Libraries/Library.h"

#include "Utility/StringPool.h"

#include <sstream>


using namespace IRSemantics;


//
// Construct and initialize a program
//
Program::Program(StringPoolManager& strings, const CompilerInfoTable& infotable)
	: CounterAnonParam(0),
	  CounterLexicalScope(0),
	  Strings(strings),
	  StructureTypeCounter(VM::EpochType_CustomBase),
	  InfoTable(infotable)
{
}

//
// Destruct and clean up a program
//
Program::~Program()
{
	for(std::map<StringHandle, Function*>::iterator iter = Functions.begin(); iter != Functions.end(); ++iter)
		delete iter->second;

	for(std::map<StringHandle, Structure*>::iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
		delete iter->second;

	for(std::vector<CodeBlock*>::iterator iter = GlobalCodeBlocks.begin(); iter != GlobalCodeBlocks.end(); ++iter)
		delete *iter;
}


//
// Add a string to the program's string pool
//
StringHandle Program::AddString(const std::wstring& str)
{
	return Strings.Pool(str);
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
		throw std::exception("Duplicate structure name");		// TODO - better exceptions
	}

	Structures.insert(std::make_pair(name, structure));
	StructureTypes.insert(std::make_pair(name, ++StructureTypeCounter));

	const std::wstring& structurename = GetString(name);
	const std::vector<std::pair<StringHandle, StructureMember*> >& members = structure->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
		AddString(GenerateStructureMemberAccessOverloadName(structurename, GetString(iter->first)));
}


//
// Allocate a function overload mangled name
//
StringHandle Program::CreateFunctionOverload(const std::wstring& name)
{
	StringHandle handle = AddString(name);
	unsigned counter = FunctionOverloadCounters[handle]++;
	if(counter == 0)
		return handle;

	std::wostringstream format;
	format << name << L"@@overload@" << counter;

	return AddString(format.str());
}


//
// Add a function definition to the program
//
void Program::AddFunction(StringHandle name, Function* function)
{
	if(Functions.find(name) != Functions.end())
	{
		delete function;
		throw std::exception("Duplicate function name");		// TODO - better exceptions
	}

	Functions.insert(std::make_pair(name, function));
}

//
// Add a global code block to a program
//
size_t Program::AddGlobalCodeBlock(CodeBlock* code)
{
	size_t index = GlobalCodeBlocks.size();
	AddString(GenerateAnonymousGlobalScopeName(index));
	GlobalCodeBlocks.push_back(code);
	return index;
}

//
// Retrieve a given global code block from the program
//
const CodeBlock& Program::GetGlobalCodeBlock(size_t index) const
{
	if(index >= GlobalCodeBlocks.size())
		throw std::exception("Invalid global code block index");		// TODO - better exceptions

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
	for(std::map<StringHandle, Structure*>::const_iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
	{
		if(!iter->second->Validate(*this))
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
	return AddString(name.str());
}

std::wstring Program::GenerateAnonymousGlobalScopeName(size_t index)
{
	std::wostringstream name;
	name << L"@@globalscope@" << index;
	return name.str();
}


StringHandle Program::AllocateLexicalScopeName(const CodeBlock* blockptr)
{
	return AddString(GenerateLexicalScopeName(blockptr));
}

StringHandle Program::FindLexicalScopeName(const CodeBlock* blockptr) const
{
	return Strings.Find(GenerateLexicalScopeName(blockptr));
}

std::wstring Program::GenerateLexicalScopeName(const CodeBlock* blockptr)
{
	std::wostringstream name;
	name << L"@@codeblock@" << blockptr;
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
		throw std::exception("Invalid structure identifier");			// TODO - better exceptions

	const std::vector<std::pair<StringHandle, StructureMember*> >& members = iter->second->GetMembers();
	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator memberiter = members.begin(); memberiter != members.end(); ++memberiter)
	{
		if(memberiter->first == membername)
			return memberiter->second->GetEpochType(*this);
	}
	
	throw std::exception("Invalid structure member");				// TODO - better exceptions
}

StringHandle Program::GetNameOfStructureType(VM::EpochTypeID structuretype) const
{
	for(std::map<StringHandle, VM::EpochTypeID>::const_iterator iter = StructureTypes.begin(); iter != StructureTypes.end(); ++iter)
	{
		if(iter->second == structuretype)
			return iter->first;
	}

	throw std::exception("Invalid structure type ID");					// TODO - better exceptions
}

StringHandle Program::FindStructureMemberAccessOverload(StringHandle structurename, StringHandle membername) const
{
	return Strings.Find(GenerateStructureMemberAccessOverloadName(GetString(structurename), GetString(membername)));
}

std::wstring Program::GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername)
{
	return L".@@" + structurename + L"@@" + membername;
}


Bytecode::EntityTag Program::GetEntityTag(StringHandle identifier) const
{
	for(EntityTable::const_iterator iter = InfoTable.Entities->begin(); iter != InfoTable.Entities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = InfoTable.ChainedEntities->begin(); iter != InfoTable.ChainedEntities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = InfoTable.PostfixEntities->begin(); iter != InfoTable.PostfixEntities->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = InfoTable.PostfixClosers->begin(); iter != InfoTable.PostfixClosers->end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	throw std::exception("Invalid entity");			// TODO - better exceptions
}

