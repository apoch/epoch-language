//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for working with compilation sessions
//

#include "pch.h"

#include "Compiler/Session.h"
#include "Compiler/SemanticActions.h"
#include "Compiler/ByteCodeEmitter.h"

#include "Parser/Parser.h"

#include "Utility/DLLPool.h"


//
// Construct a compilation session wrapper and initialize the standard library
//
CompileSession::CompileSession()
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

	CompilerInfoTable info;
	info.FunctionHelpers = &CompileTimeHelpers;
	info.InfixOperators = &InfixIdentifiers;
	info.Overloads = &FunctionOverloadNames;
	info.Precedences = &OperatorPrecedences;
	info.Entities = &CustomEntities;
	info.ChainedEntities = &ChainedEntities;
	info.PostfixEntities = &PostfixEntities;
	info.PostfixClosers = &PostfixClosers;
	bindtocompiler(info, StringPool, customtag);
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
	ByteCodeBuffer.clear();

	// All global initialization/startup code goes into the beginning of the buffer.
	// This allows the VM to just pick the start of the buffer as its current instruction
	// pointer, and then crank away.

	// Once all setup code has been executed, we invoke the entrypoint function. At this
	// point, user code is in full control of execution flow within the VM.
	ByteCodeEmitter emitter(ByteCodeBuffer);
	emitter.Invoke(StringPool.Pool(L"entrypoint"));
	emitter.Halt();

	// Now that the VM's startup stub is written out, we can begin emitting the actual
	// code for the program being compiled. This simply starts by enumerating all of the
	// user-defined functions at global scope. To accomplish this, we will traverse the
	// set of code blocks that have been provided, and compile each one's global-scoped
	// functions.
	for(std::list<std::pair<std::wstring, std::wstring> >::const_iterator iter = SourceBlocksAndFileNames.begin(); iter != SourceBlocksAndFileNames.end(); ++iter)
		CompileFunctions(iter->first, iter->second);
}


//
// Retrieve the actual bytecode buffer generated during compilation
//
const void* CompileSession::GetEmittedBuffer() const
{
	return &ByteCodeBuffer[0];
}

//
// Retrieve the size of the bytecode buffer generated during compilation
//
size_t CompileSession::GetEmittedBufferSize() const
{
	return ByteCodeBuffer.size();
}


//
// Compile all global-scoped functions in the given code block
//
void CompileSession::CompileFunctions(const std::wstring& code, const std::wstring& filename)
{
	std::set<std::wstring> entitynames, chainedentitynames, postfixentitynames, postfixclosernames;
	for(EntityTable::iterator iter = CustomEntities.begin(); iter != CustomEntities.end(); ++iter)
		entitynames.insert(StringPool.GetPooledString(iter->second.StringName));

	for(EntityTable::iterator iter = ChainedEntities.begin(); iter != ChainedEntities.end(); ++iter)
		chainedentitynames.insert(StringPool.GetPooledString(iter->second.StringName));

	for(EntityTable::iterator iter = PostfixEntities.begin(); iter != PostfixEntities.end(); ++iter)
		postfixentitynames.insert(StringPool.GetPooledString(iter->second.StringName));

	for(EntityTable::iterator iter = PostfixClosers.begin(); iter != PostfixClosers.end(); ++iter)
		postfixclosernames.insert(StringPool.GetPooledString(iter->second.StringName));

	ByteCodeEmitter emitter(ByteCodeBuffer);
	CompilationSemantics semantics(emitter, *this);
	Parser theparser(semantics, InfixIdentifiers, entitynames, chainedentitynames, postfixentitynames, postfixclosernames);

	if(!theparser.Parse(code, filename) || semantics.DidFail())
		throw FatalException("Parsing failed!");

	semantics.SanityCheck();
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