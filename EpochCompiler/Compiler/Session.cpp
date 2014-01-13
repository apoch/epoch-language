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
#include "Compiler/Passes/ParseCallbacks.h"
#include "Compiler/Passes/Optimization.h"
#include "Compiler/Passes/ASTPlugin.h"

#include "Compiler/Self Hosting Plugins/Plugin.h"

#include "Parser/Parser.h"

#include "Utility/DLLPool.h"
#include "Utility/Profiling.h"

#include "Metadata/Precedences.h"

#include "User Interface/Output.h"


//
// Construct a compilation session wrapper and initialize the standard library
//
CompileSession::CompileSession()
	: ASTProgram(NULL),
	  SemanticProgram(NULL),
	  StringPool(true)
{
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochLibrary.DLL");

	typedef void (STDCALL *registerlibraryptr)(FunctionSignatureSet&, StringPoolManager&);
	registerlibraryptr registerlibrary = Marshaling::DLLPool::GetFunction<registerlibraryptr>(dllhandle, "RegisterLibraryContents");

	typedef void (STDCALL *bindtocompilerptr)(CompilerInfoTable&, StringPoolManager&, Bytecode::EntityTag&);
	bindtocompilerptr bindtocompiler = Marshaling::DLLPool::GetFunction<bindtocompilerptr>(dllhandle, "BindToCompiler");

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

	InitIntrinsicTypes();

	if(Plugins.IsPluginFunctionProvided(L"PluginOnCompileStart"))
		Plugins.InvokeVoidPluginFunction(L"PluginOnCompileStart");
}

//
// Destruct and clean up a compilation session
//
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

	FinalByteCode.clear();

	if(Plugins.IsPluginFunctionProvided(L"PluginBytecodeGetBuffer"))
	{
		// Pre-cache strings that are always present (hack until the compiler does this automatically)
		const std::map<StringHandle, PooledString>& pool = StringPool.GetInternalPool();
		for(std::map<StringHandle, PooledString>::const_iterator iter = pool.begin(); iter != pool.end(); ++iter)
			Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterString", iter->first, iter->second.Data.c_str());

		for(std::list<std::pair<std::wstring, std::wstring> >::const_iterator iter = SourceBlocksAndFileNames.begin(); iter != SourceBlocksAndFileNames.end(); ++iter)
			CompileFileToPlugin(iter->first, iter->second);

		BytecodeEmitterPlugin emitter;
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenProcessProgram");

		const Byte* p = emitter.Buffer.GetPointer();
		std::vector<Byte>(p, p + emitter.Buffer.GetSize()).swap(FinalByteCode);
	}
	else
	{
		for(std::list<std::pair<std::wstring, std::wstring> >::const_iterator iter = SourceBlocksAndFileNames.begin(); iter != SourceBlocksAndFileNames.end(); ++iter)
			CompileFile(iter->first, iter->second);

		UI::OutputStream output;

		Profiling::Timer timer;

		output << L"Optimizing IR... ";
		timer.Begin();
		CompilerPasses::Optimize(*SemanticProgram);
		timer.End();
		output << L"finished in " << timer.GetTimeMs() << L"ms" << std::endl;

		timer.Begin();

		output << L"Generating code... ";

		BytecodeStreamVector stream;
		ByteCodeEmitter emitter(stream);
		CompilerPasses::GenerateCode(*SemanticProgram, emitter);

		std::vector<Byte>(stream.GetPointer(), stream.GetPointer() + stream.GetSize()).swap(FinalByteCode);

		timer.End();
		output << L"finished in " << timer.GetTimeMs() << L"ms" << std::endl;
	}
}


//
// Retrieve the actual bytecode buffer generated during compilation
//
void* CompileSession::GetEmittedBuffer() const
{
	if(FinalByteCode.empty())
		throw std::runtime_error("Empty bytecode buffer");

	return const_cast<void*>(reinterpret_cast<const void*>(&FinalByteCode[0]));
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

	if(!theparser.Parse(code, filename, ASTProgram))
		throw FatalException("Parsing failed!");

	delete SemanticProgram;
	SemanticProgram = NULL;

	FileName = filename;

	UI::OutputStream output;
	output << L"Validating semantics... ";
	Profiling::Timer timer;
	timer.Begin();

	SemanticProgram = CompilerPasses::ValidateSemantics(*ASTProgram, code.begin(), code.end(), StringPool, *this);

	timer.End();
	output << L"finished in " << timer.GetTimeMs() << L"ms" << std::endl;

	if(!SemanticProgram)
		throw FatalException("Semantic checking failed!");
}


void CompileSession::CompileFileToPlugin(const std::wstring& code, const std::wstring& filename)
{
	Parser theparser(Identifiers); 

	if(!theparser.Parse(code, filename, ASTProgram))
		throw FatalException("Parsing failed!");

	delete SemanticProgram;
	SemanticProgram = NULL;

	FileName = filename;

	UI::OutputStream output;
	output << L"Validating semantics... ";
	Profiling::Timer timer;
	timer.Begin();

	CompilerPasses::HandASTToPlugin(*ASTProgram);

	timer.End();
	output << L"finished in " << timer.GetTimeMs() << L"ms" << std::endl;
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

//
// Initialize the table of intrinsic type ID/name mappings
//
void CompileSession::InitIntrinsicTypes()
{
	IntrinsicTypes[StringPool.Pool(L"integer")] = Metadata::EpochType_Integer;
	IntrinsicTypes[StringPool.Pool(L"integer16")] = Metadata::EpochType_Integer16;
	IntrinsicTypes[StringPool.Pool(L"string")] = Metadata::EpochType_String;
	IntrinsicTypes[StringPool.Pool(L"boolean")] = Metadata::EpochType_Boolean;
	IntrinsicTypes[StringPool.Pool(L"real")] = Metadata::EpochType_Real;
	IntrinsicTypes[StringPool.Pool(L"buffer")] = Metadata::EpochType_Buffer;
	IntrinsicTypes[StringPool.Pool(L"identifier")] = Metadata::EpochType_Identifier;
	IntrinsicTypes[StringPool.Pool(L"nothing")] = Metadata::EpochType_Nothing;
	IntrinsicTypes[StringPool.Pool(L"function")] = Metadata::EpochTypeFamily_Function;
}

//
// Parse a source block and invoke the given callbacks
//
void CompileSession::Parse(const std::wstring& code, const std::wstring& filename, const ParseCallbackTable& callbacks)
{
	Parser theparser(Identifiers);

	if(!theparser.Parse(code, filename, ASTProgram))
		return;

	callbacks.ParseOK();
	
	CompilerPasses::ParseWithCallbacks(*ASTProgram, callbacks);
}

