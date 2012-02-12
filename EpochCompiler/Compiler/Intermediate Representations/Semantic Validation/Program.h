//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a complete Epoch program
//

#pragma once


// Dependencies
#include "Compiler/ExportDef.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Bytecode/EntityTags.h"

#include "Metadata/ScopeDescription.h"

#include <vector>
#include <map>
#include <boost/unordered_map.hpp>


// Forward declarations
class StringPoolManager;
class CompileSession;


namespace IRSemantics
{

	// Forward declarations
	class Function;
	class Structure;
	class CodeBlock;


	typedef std::map<StringHandle, const ScopeDescription*> ScopePtrMap;


	namespace impl
	{
		template<typename T>
		class StringCache
		{
		private:
			typedef boost::unordered_map<T, StringHandle> CacheType;

		public:
			StringHandle Find(const T& key) const
			{
                typename CacheType::const_iterator iter = Cache.find(key);
				if(iter == Cache.end())
					return 0;

				return iter->second;
			}

			void Add(const T& key, StringHandle str)
			{
				Cache[key] = str;
			}

		private:
			CacheType Cache;
		};
	}


	//
	// Container for all contents of an Epoch program
	//
	class Program
	{
	// Construction and destruction
	public:
		Program(StringPoolManager& strings, CompileSession& session);
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
		StringHandle FindLexicalScopeName(const ScopeDescription* scopeptr) const;

	// Manipulation of associated functions
	public:
		StringHandle CreateFunctionOverload(const std::wstring& name);

		void AddFunction(StringHandle name, Function* function);
		bool HasFunction(StringHandle name) const;

		const boost::unordered_map<StringHandle, Function*>& GetFunctions() const
		{ return Functions; }

	// Access to lexical scopes
	public:
		const ScopePtrMap& GetScopes() const
		{ return LexicalScopes; }

		void AddScope(ScopeDescription* scope);
		void AddScope(ScopeDescription* scope, StringHandle name);

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

	// Type inference
	public:
		bool TypeInference();
		InferenceContext::PossibleParameterTypes GetExpectedTypesForStatement(StringHandle name) const;

	// Validation
	public:
		bool Validate() const;

	// Internal helpers
	private:
		std::wstring GenerateFunctionOverloadName(StringHandle name, size_t index) const;
		static std::wstring GenerateAnonymousGlobalScopeName(size_t index);
		static std::wstring GenerateLexicalScopeName(const ScopeDescription* scopeptr);
		static std::wstring GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername);

	// Accessible state
	public:
		CompileSession& Session;

	// Internal state
	private:
		StringPoolManager& Strings;

		std::map<StringHandle, Structure*> Structures;
		std::map<StringHandle, VM::EpochTypeID> StructureTypes;
		VM::EpochTypeID StructureTypeCounter;

		boost::unordered_map<StringHandle, Function*> Functions;
		boost::unordered_map<StringHandle, unsigned> FunctionOverloadCounters;

		std::vector<CodeBlock*> GlobalCodeBlocks;

		unsigned CounterAnonParam;
		unsigned CounterLexicalScope;

		ScopePtrMap LexicalScopes;

		ScopeDescription* GlobalScope;

	// String lookup caches
	private:
		impl::StringCache<std::pair<StringHandle, size_t> > FunctionOverloadNameCache;
		impl::StringCache<std::pair<StringHandle, StringHandle> > StructureMemberAccessOverloadNameCache;
		impl::StringCache<const ScopeDescription*> LexicalScopeNameCache;
		impl::StringCache<std::wstring> IdentifierCache;
	};

}

