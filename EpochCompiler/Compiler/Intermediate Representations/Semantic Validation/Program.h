//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a complete Epoch program
//

#pragma once


// Dependencies
#include "Compiler/ExportDef.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Bytecode/EntityTags.h"

#include "Metadata/ScopeDescription.h"

#include <vector>
#include <map>


// Forward declarations
class StringPoolManager;
struct CompilerInfoTable;


namespace IRSemantics
{

	// Forward declarations
	class Function;
	class Structure;
	class CodeBlock;


	//
	// Container for all contents of an Epoch program
	//
	class Program
	{
	// Construction and destruction
	public:
		Program(StringPoolManager& strings, CompilerInfoTable& infotable);
		~Program();

	// Non-copyable
	private:
		Program(const Program& other);
		Program& operator = (const Program& rhs);

	// String pooling
	public:
		StringHandle AddString(const std::wstring& str);
		EPOCHCOMPILER const std::wstring& GetString(StringHandle handle) const;
		
		const StringPoolManager& GetStringPool() const
		{ return Strings; }

	// Automatic allocation of dummy identifiers
	public:
		StringHandle AllocateAnonymousParamName();

		StringHandle AllocateLexicalScopeName(const CodeBlock* blockptr);
		StringHandle FindLexicalScopeName(const CodeBlock* blockptr) const;

	// Manipulation of associated functions
	public:
		StringHandle CreateFunctionOverload(const std::wstring& name);

		void AddFunction(StringHandle name, Function* function);

		const std::map<StringHandle, Function*>& GetFunctions() const
		{ return Functions; }

	// Manipulation of associated structures
	public:
		void AddStructure(StringHandle name, Structure* structure);

		const std::map<StringHandle, Structure*>& GetStructures() const
		{ return Structures; }

		StringHandle GetNameOfStructureType(VM::EpochTypeID type) const;
		VM::EpochTypeID GetStructureMemberType(StringHandle structurename, StringHandle membername) const;
		StringHandle FindStructureMemberAccessOverload(StringHandle structurename, StringHandle membername) const;

	// Manipulation of associated global code blocks
	public:
		size_t AddGlobalCodeBlock(CodeBlock* code);
		const CodeBlock& GetGlobalCodeBlock(size_t index) const;
		size_t GetNumGlobalCodeBlocks() const;
		StringHandle GetGlobalCodeBlockName(size_t index) const;

		ScopeDescription* GetGlobalScope();

	// Type alias lookup
	public:
		EPOCHCOMPILER VM::EpochTypeID LookupType(StringHandle name) const;

	// Entities
	public:
		Bytecode::EntityTag GetEntityTag(StringHandle entityname) const;

	// Compile-time code execution
	public:
		bool CompileTimeCodeExecution();

	// Validation
	public:
		bool Validate() const;

	// Internal helpers
	private:
		static std::wstring GenerateAnonymousGlobalScopeName(size_t index);
		static std::wstring GenerateLexicalScopeName(const CodeBlock* blockptr);
		static std::wstring GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername);

	// Accessible state
	public:
		CompilerInfoTable& InfoTable;

	// Internal state
	private:
		StringPoolManager& Strings;

		std::map<StringHandle, Structure*> Structures;
		std::map<StringHandle, VM::EpochTypeID> StructureTypes;
		VM::EpochTypeID StructureTypeCounter;

		std::map<StringHandle, Function*> Functions;
		std::map<StringHandle, unsigned> FunctionOverloadCounters;

		std::vector<CodeBlock*> GlobalCodeBlocks;

		unsigned CounterAnonParam;
		unsigned CounterLexicalScope;

		ScopeMap LexicalScopes;

		ScopeDescription* GlobalScope;
	};

}

