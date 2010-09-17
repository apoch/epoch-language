//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for working with compilation sessions
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/StringPool.h"
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"
#include <list>


//
// A compilation session maps one program's source code to one executable
// binary bytecode buffer. Note that there may be multiple source code
// buffers/files/etc. involved in a single "program." However, there will
// only ever be a single resolved output bytecode buffer.
//
class CompileSession
{
// Construction
public:
	CompileSession();

// Tracking of code blocks to compile
public:
	void AddCompileBlock(const std::wstring& source);

// Bytecode generation
public:
	void EmitByteCode();

// Access to compiled bytecode
public:
	const void* GetEmittedBuffer() const;
	size_t GetEmittedBufferSize() const;

// Publicly visible tracking
public:
	FunctionSignatureSet FunctionSignatures;
	FunctionCompileHelperTable CompileTimeHelpers;
	StringPoolManager StringPool;

// Internal helpers
private:
	void CompileFunctions(const std::wstring& codeblock);

// Internal tracking
private:
	std::list<std::wstring> SourceBlocks;
	std::vector<Byte> ByteCodeBuffer;
};

