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

	typedef void (__stdcall *bindtocompilerptr)(FunctionCompileHelperTable&, InfixTable&, StringPoolManager&);
	bindtocompilerptr bindtocompiler = reinterpret_cast<bindtocompilerptr>(::GetProcAddress(dllhandle, "BindToCompiler"));

	if(!registerlibrary || !bindtocompiler)
		throw FatalException("Failed to load Epoch standard library");

	registerlibrary(FunctionSignatures, StringPool);
	bindtocompiler(CompileTimeHelpers, InfixIdentifiers, StringPool);
}


//
// Add a code block to the compilation session
//
void CompileSession::AddCompileBlock(const std::wstring& source)
{
	SourceBlocks.push_back(source);
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
	for(std::list<std::wstring>::const_iterator iter = SourceBlocks.begin(); iter != SourceBlocks.end(); ++iter)
		CompileFunctions(*iter);
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
void CompileSession::CompileFunctions(const std::wstring& code)
{
	ByteCodeEmitter emitter(ByteCodeBuffer);
	CompilationSemantics semantics(emitter, *this);
	Parser theparser(semantics, InfixIdentifiers);

	if(!theparser.Parse(code) || semantics.DidFail())
		throw FatalException("Parsing failed!");

	semantics.SanityCheck();
}

