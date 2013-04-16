//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for working with compilation sessions
//

#pragma once


// Forward declarations
namespace AST { struct Program; }
namespace IRSemantics { class Program; }


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/StringPool.h"

#include "Metadata/FunctionSignature.h"

#include "Libraries/Library.h"

#include "Bytecode/EntityTags.h"

#include <list>


// Handy type shortcuts
typedef std::vector<char> ByteBuffer;
typedef std::map<StringHandle, Metadata::EpochTypeID> NameToTypeTable;


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
	~CompileSession();

// Tracking of code blocks to compile
public:
	void AddCompileBlock(const std::wstring& source, const std::wstring& filename);

// Bytecode generation
public:
	void EmitByteCode();

// Access to compiled bytecode
public:
	void* GetEmittedBuffer() const;
	size_t GetEmittedBufferSize() const;

// Entity management
public:
	const EntityDescription& GetCustomEntityByName(StringHandle name) const;
	const EntityDescription& GetCustomEntityByTag(Bytecode::EntityTag tag) const;

// Parsing hooks
public:
	struct ParseCallbackTable
	{
		typedef void (__stdcall *ParseOKPtr)();
		typedef void (__stdcall *ParseStructurePtr)(const wchar_t* name);

		ParseOKPtr ParseOK;
		ParseStructurePtr ParseStructure;
	};

	void Parse(const std::wstring& code, const std::wstring& filename, const ParseCallbackTable& callbacks);

// Publicly visible tracking
public:
	FunctionSignatureSet FunctionSignatures;
	OverloadMap FunctionOverloadNames;
	FunctionCompileHelperTable CompileTimeHelpers;
	StringPoolManager StringPool;
	PrecedenceTable OperatorPrecedences;
	EntityTable CustomEntities;
	EntityTable ChainedEntities;
	EntityTable PostfixEntities;
	EntityTable PostfixClosers;
	FunctionTagHelperTable FunctionTagHelpers;
	CompilerInfoTable InfoTable;

	IdentifierTable Identifiers;

	NameToTypeTable IntrinsicTypes;

	std::wstring FileName;

// Internal helpers
private:
	void CompileFile(const std::wstring& codeblock, const std::wstring& filename);

	void InitIntrinsicTypes();

// Internal tracking
private:
	std::list<std::pair<std::wstring, std::wstring> > SourceBlocksAndFileNames;
	
	AST::Program* ASTProgram;
	IRSemantics::Program* SemanticProgram;
	ByteBuffer FinalByteCode;
};

