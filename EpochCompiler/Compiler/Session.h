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

#include "Bytecode/EntityTags.h"

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
	void AddCompileBlock(const std::wstring& source, const std::wstring& filename);

// Bytecode generation
public:
	void EmitByteCode();

// Access to compiled bytecode
public:
	const void* GetEmittedBuffer() const;
	size_t GetEmittedBufferSize() const;

// Entity management
public:
	const EntityDescription& GetCustomEntityByTag(Bytecode::EntityTag tag) const;

// Publicly visible tracking
public:
	FunctionSignatureSet FunctionSignatures;
	std::map<StringHandle, std::set<StringHandle> > FunctionOverloadNames;
	FunctionCompileHelperTable CompileTimeHelpers;
	StringPoolManager StringPool;
	std::multimap<int, StringHandle> OperatorPrecedences;
	EntityTable CustomEntities;

// Internal helpers
private:
	void CompileFunctions(const std::wstring& codeblock, const std::wstring& filename);

// Internal tracking
private:
	std::list<std::pair<std::wstring, std::wstring> > SourceBlocksAndFileNames;
	std::vector<Byte> ByteCodeBuffer;
	InfixTable InfixIdentifiers;
};

