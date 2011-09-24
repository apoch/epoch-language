//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for working with compilation sessions
//

#include "pch.h"

#include "Compiler/Session.h"
#include "Compiler/ByteCodeEmitter.h"

#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"
#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Function.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Passes/SemanticValidation.h"
#include "Compiler/Passes/CodeGeneration.h"

#include "Parser/Parser.h"

#include "Utility/DLLPool.h"
#include "Utility/Profiling.h"

#include "Metadata/Precedences.h"

#include "Compiler/Diagnostics.h"

#include <iostream>


//
// Construct a compilation session wrapper and initialize the standard library
//
CompileSession::CompileSession()
	: ASTProgram(NULL),
	  SemanticProgram(NULL)
{
	HINSTANCE dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochLibrary.DLL");

	typedef void (__stdcall *registerlibraryptr)(FunctionSignatureSet&, StringPoolManager&);
	registerlibraryptr registerlibrary = reinterpret_cast<registerlibraryptr>(::GetProcAddress(dllhandle, "RegisterLibraryContents"));

	typedef void (__stdcall *bindtocompilerptr)(CompilerInfoTable&, StringPoolManager&, Bytecode::EntityTag&);
	bindtocompilerptr bindtocompiler = reinterpret_cast<bindtocompilerptr>(::GetProcAddress(dllhandle, "BindToCompiler"));

	if(!registerlibrary || !bindtocompiler)
		throw FatalException("Failed to load Epoch standard library");

	registerlibrary(FunctionSignatures, StringPool);

	Bytecode::EntityTag customtag = Bytecode::EntityTags::CustomEntityBaseID;

	InfoTable.FunctionHelpers = &CompileTimeHelpers;
	InfoTable.InfixOperators = &Identifiers.InfixOperators;
	InfoTable.OpAssignOperators = &Identifiers.OpAssignmentIdentifiers;
	InfoTable.UnaryPrefixes = &Identifiers.UnaryPrefixes;
	InfoTable.PreOperators = &Identifiers.PreOperators;
	InfoTable.PostOperators = &Identifiers.PostOperators;
	InfoTable.Overloads = &FunctionOverloadNames;
	InfoTable.Precedences = &OperatorPrecedences;
	InfoTable.Entities = &CustomEntities;
	InfoTable.ChainedEntities = &ChainedEntities;
	InfoTable.PostfixEntities = &PostfixEntities;
	InfoTable.PostfixClosers = &PostfixClosers;
	InfoTable.FunctionTagHelpers = &FunctionTagHelpers;
	bindtocompiler(InfoTable, StringPool, customtag);

	for(EntityTable::const_iterator iter = CustomEntities.begin(); iter != CustomEntities.end(); ++iter)
		Identifiers.CustomEntities.insert(StringPool.GetPooledString(iter->second.StringName));

	for(EntityTable::const_iterator iter = ChainedEntities.begin(); iter != ChainedEntities.end(); ++iter)
		Identifiers.ChainedEntities.insert(StringPool.GetPooledString(iter->second.StringName));

	for(EntityTable::const_iterator iter = PostfixEntities.begin(); iter != PostfixEntities.end(); ++iter)
		Identifiers.PostfixEntities.insert(StringPool.GetPooledString(iter->second.StringName));

	for(EntityTable::const_iterator iter = PostfixClosers.begin(); iter != PostfixClosers.end(); ++iter)
		Identifiers.PostfixClosers.insert(StringPool.GetPooledString(iter->second.StringName));

	OperatorPrecedences.insert(std::make_pair(PRECEDENCE_MEMBERACCESS, StringPool.Pool(L".")));
}

CompileSession::~CompileSession()
{
	delete ASTProgram;
	delete SemanticProgram;
}


//
// Add a code block to the compilation session
//
void CompileSession::AddCompileBlock(const std::wstring& source, const std::wstring& filename)
{
	SourceBlocksAndFileNames.push_back(std::make_pair(source, filename));
}


//
// Compile the given source block(s) into bytecode
//
void CompileSession::EmitByteCode()
{
	delete ASTProgram;
	ASTProgram = new AST::Program;

	FinalByteCode.clear();

	for(std::list<std::pair<std::wstring, std::wstring> >::const_iterator iter = SourceBlocksAndFileNames.begin(); iter != SourceBlocksAndFileNames.end(); ++iter)
		CompileFile(iter->first, iter->second);

	Profiling::Timer timer;
	timer.Begin();

	std::wcout << L"Generating code... ";

	ByteCodeEmitter emitter(FinalByteCode);
	CompilerPasses::GenerateCode(*SemanticProgram, emitter);

	timer.End();
	std::wcout << L"finished in " << timer.GetTimeMs() << L"ms" << std::endl;
}


//
// Retrieve the actual bytecode buffer generated during compilation
//
const void* CompileSession::GetEmittedBuffer() const
{
	if(FinalByteCode.empty())
		throw std::exception("Empty bytecode buffer");

	return &FinalByteCode[0];
}

//
// Retrieve the size of the bytecode buffer generated during compilation
//
size_t CompileSession::GetEmittedBufferSize() const
{
	return FinalByteCode.size();
}


//
// Compile all source in the given code block
//
void CompileSession::CompileFile(const std::wstring& code, const std::wstring& filename)
{
	Parser theparser(Identifiers);

	if(!theparser.Parse(code, filename, *ASTProgram))
		throw FatalException("Parsing failed!");

	delete SemanticProgram;
	SemanticProgram = NULL;

	std::wcout << L"Validating semantics... ";
	Profiling::Timer timer;
	timer.Begin();

	SemanticProgram = CompilerPasses::ValidateSemantics(*ASTProgram, StringPool, InfoTable);

	timer.End();
	std::wcout << L"finished in " << timer.GetTimeMs() << L"ms" << std::endl;

	if(!SemanticProgram)
		throw FatalException("Semantic checking failed!");
}


//
// Retrieve the entity descriptor of the entity type with the given tag
//
const EntityDescription& CompileSession::GetCustomEntityByTag(Bytecode::EntityTag tag) const
{
	EntityTable::const_iterator iter = CustomEntities.find(tag);
	if(iter != CustomEntities.end())
		return iter->second;

	iter = ChainedEntities.find(tag);
	if(iter != ChainedEntities.end())
		return iter->second;

	iter = PostfixEntities.find(tag);
	if(iter != PostfixEntities.end())
		return iter->second;

	iter = PostfixClosers.find(tag);
	if(iter != PostfixClosers.end())
		return iter->second;

	throw FatalException("Invalid entity tag - could not retrieve descriptor");
}

//
// Retrieve the entity descriptor of the entity type with the given name
//
const EntityDescription& CompileSession::GetCustomEntityByName(StringHandle name) const
{
	for(EntityTable::const_iterator iter = CustomEntities.begin(); iter != CustomEntities.end(); ++iter)
	{
		if(iter->second.StringName == name)
			return iter->second;
	}

	for(EntityTable::const_iterator iter = ChainedEntities.begin(); iter != ChainedEntities.end(); ++iter)
	{
		if(iter->second.StringName == name)
			return iter->second;
	}

	for(EntityTable::const_iterator iter = PostfixEntities.begin(); iter != PostfixEntities.end(); ++iter)
	{
		if(iter->second.StringName == name)
			return iter->second;
	}

	for(EntityTable::const_iterator iter = PostfixClosers.begin(); iter != PostfixClosers.end(); ++iter)
	{
		if(iter->second.StringName == name)
			return iter->second;
	}

	throw InvalidIdentifierException("Invalid entity name");
}

