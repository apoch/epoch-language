//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR management objects for namespaces
//

#pragma once


#include "Utility/Types/IDTypes.h"

#include "Metadata/FunctionSignature.h"

#include "Bytecode/EntityTags.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"
#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"
#include "Compiler/Intermediate Representations/Semantic Validation/TypeSpace.h"

#include "Compiler/ExportDef.h"

#include "Libraries/Library.h"

#include <boost/unordered_map.hpp>


class ScopeDescription;
class CompileErrors;
class StringPoolManager;
class CompileSession;


namespace IRSemantics
{

	struct InferenceContext;
	class CodeBlock;
	class Function;
	class Structure;
	class GlobalIDSpace;
	class Namespace;


	typedef std::map<StringHandle, const ScopeDescription*> ScopePtrMap;


	//
	// Wrapper for holding all functions in a namespace
	//
	class FunctionTable
	{
	// Handy type shortcuts
	public:
		typedef std::map<StringHandle, CompileTimeParameterVector> InstancesAndArguments;
		typedef std::map<StringHandle, InstancesAndArguments> InstantiationMap;

	// Construction and destruction
	public:
		FunctionTable(Namespace& mynamespace, CompileSession& session);
		~FunctionTable();

	// Non-copyable
	private:
		FunctionTable(const FunctionTable&);
		FunctionTable& operator = (const FunctionTable&);

	// General function management interface
	public:
		void Add(StringHandle name, StringHandle rawname, Function* function);
		EPOCHCOMPILER bool IRExists(StringHandle functionname) const;

	// Function signature management interface
	public:
		bool SignatureExists(StringHandle functionname) const;

		const FunctionSignature& GetSignature(StringHandle functionname) const;
		void SetSignature(StringHandle functionname, const FunctionSignature& signature);

	// Overload management interface
	public:
		bool HasOverloads(StringHandle functionname) const;
		const StringHandleSet& GetOverloadNames(StringHandle functionname) const;

		StringHandle CreateOverload(const std::wstring& name);
		void AddInternalOverload(StringHandle functionname, StringHandle overloadname);

		unsigned GetNumOverloads(StringHandle name) const;
		StringHandle GetOverloadName(StringHandle rawname, unsigned overloadindex) const;

	// Structures support
	public:
		void GenerateStructureFunctions(StringHandle name, Structure* structure);
		StringHandle FindStructureMemberAccessOverload(StringHandle structurename, StringHandle membername) const;

	// Sum type support
	public:
		void GenerateSumTypeFunctions(Metadata::EpochTypeID sumtypeid, const std::set<StringHandle>& basetypenames);

	// Compiler helper interface
	public:
		bool HasCompileHelper(StringHandle functionname) const;
		CompilerHelperPtr GetCompileHelper(StringHandle functionname) const;
		void SetCompileHelper(StringHandle functionname, CompilerHelperPtr helper);

	// IR access
	public:
		Function* GetIR(StringHandle functionname);
		const Function* GetIR(StringHandle functionname) const;

	// Pattern and type matching
	public:
		void MarkFunctionWithStaticPatternMatching(StringHandle rawname, StringHandle overloadname);
		bool FunctionNeedsDynamicPatternMatching(StringHandle overloadname) const;
		StringHandle GetDynamicPatternMatcherForFunction(StringHandle overloadname) const;

		const std::map<StringHandle, std::map<StringHandle, FunctionSignature> >& GetRequiredTypeMatchers() const
		{ return RequiredTypeMatchers; }

		StringHandle AllocateTypeMatcher(StringHandle overloadname, const std::map<StringHandle, FunctionSignature>& matchingoverloads);

	// Semantic analysis pass
	public:
		bool Validate(CompileErrors& errors) const;
		bool TypeInference(InferenceContext& context, CompileErrors& errors);
		bool CompileTimeCodeExecution(CompileErrors& errors);

	// Function template support
	public:
		bool IsFunctionTemplate(StringHandle name) const;
		StringHandle InstantiateAllOverloads(StringHandle templatename, const CompileTimeParameterVector& args, CompileErrors& errors);

	// Additional queries
	public:
		const boost::unordered_map<StringHandle, Function*>& GetDefinitions() const
		{ return FunctionIR; }

		const std::set<StringHandle>& GetFunctionsNeedingDynamicPatternMatching() const
		{ return FunctionsNeedingDynamicPatternMatching; }

		InferenceContext::PossibleParameterTypes GetExpectedTypes(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors& errors) const;
		InferenceContext::PossibleSignatureSet GetExpectedSignatures(StringHandle name, const ScopeDescription& scope, StringHandle contextname, CompileErrors& errors) const;

		unsigned FindMatchingFunctions(StringHandle identifier, const FunctionSignature& expectedsignature, InferenceContext& context, CompileErrors& errors, StringHandle& resolvedidentifier);

	// Internal helpers
	private:
		StringHandle InstantiateTemplate(StringHandle templatename, const CompileTimeParameterVector& args, CompileErrors& errors);

		std::wstring GenerateFunctionOverloadName(StringHandle name, size_t index) const;
		static std::wstring GenerateStructureMemberAccessOverloadName(const std::wstring& structurename, const std::wstring& membername);
		static std::wstring GeneratePatternMatcherName(const std::wstring& funcname);

	// Internal tracking
	private:
		Namespace& MyNamespace;
		CompileSession& Session;

		boost::unordered_map<StringHandle, Function*> FunctionIR;
		boost::unordered_map<StringHandle, unsigned> FunctionOverloadCounters;
		
		InstantiationMap Instantiations;

		std::map<StringHandle, StringHandle> OverloadTypeMatchers;
		std::map<StringHandle, std::map<StringHandle, FunctionSignature> > RequiredTypeMatchers;

		std::multimap<StringHandle, StringHandle> FunctionsWithStaticPatternMatching;
		std::set<StringHandle> FunctionsNeedingDynamicPatternMatching;

		impl::StringCache<std::pair<StringHandle, size_t> > FunctionOverloadNameCache;
		impl::StringCache<std::pair<StringHandle, StringHandle> > StructureMemberAccessOverloadNameCache;
		impl::StringCache<StringHandle> PatternMatcherNameCache;
	};


	//
	// Wrapper for holding all function tags valid in a namespace
	//
	class FunctionTagTable
	{
	// Construction
	public:
		explicit FunctionTagTable(const CompileSession& session);

	// Non-copyable
	private:
		FunctionTagTable(const FunctionTagTable&);
		FunctionTagTable& operator = (const FunctionTagTable&);

	// Function tag management interface
	public:
		bool Exists(StringHandle tagname) const;
		FunctionTagHelperPtr GetHelper(StringHandle tagname) const;

	// Internal tracking
	private:
		const CompileSession& Session;
	};


	//
	// Wrapper for holding all operators valid in a namespace
	//
	class OperatorTable
	{
	// Construction
	public:
		OperatorTable(Namespace& mynamespace, const CompileSession& session);

	// Non-copyable
	private:
		OperatorTable(const OperatorTable&);
		OperatorTable& operator = (const OperatorTable&);

	// Prefix management interface
	public:
		bool PrefixExists(StringHandle prefixname) const;

	// Additional queries
	public:
		int GetPrecedence(StringHandle operatorname) const;

	// Internal tracking
	private:
		Namespace& MyNamespace;
		const CompileSession& Session;
	};


	//
	// Wrapper for holding all content valid in a namespace
	//
	class Namespace
	{
	// Construction and destruction
	public:
		Namespace(GlobalIDSpace& idspace, StringPoolManager& strings, CompileSession& session);
		~Namespace();

	// Non-copyable
	private:
		Namespace(const Namespace&);
		Namespace& operator = (const Namespace&);

	// Components
	public:
		TypeSpace Types;
		FunctionTable Functions;
		FunctionTagTable FunctionTags;
		OperatorTable Operators;

	// Linkages
	public:
		StringPoolManager& Strings;

	// Access to lexical scopes
	public:
		const ScopePtrMap& GetScopes() const
		{ return LexicalScopes; }

		void AddScope(ScopeDescription* scope);
		void AddScope(ScopeDescription* scope, StringHandle name);

	// Automatic allocation of dummy identifiers
	public:
		StringHandle AllocateAnonymousParamName();

		StringHandle AllocateLexicalScopeName(const ScopeDescription* scopeptr);
		StringHandle FindLexicalScopeName(const CodeBlock* blockptr) const;
		StringHandle FindLexicalScopeName(const ScopeDescription* scopeptr) const;

	// Manipulation of associated global code blocks
	public:
		size_t AddGlobalCodeBlock(CodeBlock* code);
		const CodeBlock& GetGlobalCodeBlock(size_t index) const;
		size_t GetNumGlobalCodeBlocks() const;
		StringHandle GetGlobalCodeBlockName(size_t index) const;

		ScopeDescription* GetGlobalScope() const;

	// Entities
	public:
		Bytecode::EntityTag GetEntityTag(StringHandle entityname) const;
		Bytecode::EntityTag GetEntityCloserTag(StringHandle entityname) const;

	// Semantic analysis pass
	public:
		bool Validate(CompileErrors& errors) const;
		bool TypeInference(CompileErrors& errors);
		bool CompileTimeCodeExecution(CompileErrors& errors);

	// Control of parent namespace
	public:
		void SetParent(Namespace* parent);

	// Dummy namespaces for template function instantiations
	public:
		static Namespace* CreateTemplateDummy(Namespace& parent, const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args);

	// Shadowing checking
	public:
		EPOCHCOMPILER bool ShadowingCheck(StringHandle identifier, CompileErrors& errors);

	// Internal helpers
	private:
		static std::wstring GenerateAnonymousGlobalScopeName(size_t index);
		static std::wstring GenerateLexicalScopeName(const ScopeDescription* scopeptr);

	// Internal tracking
	private:
		friend class TypeSpace;
		friend class FunctionTable;
		friend class FunctionSignatureTable;
		friend class TemplateTable;
		friend class StructureTable;
		friend class SumTypeTable;
		friend class Entity;
		friend class Structure;

		CompileSession& Session;

		std::vector<CodeBlock*> GlobalCodeBlocks;

		unsigned CounterAnonParam;
		unsigned CounterLexicalScope;

		ScopePtrMap LexicalScopes;

		ScopeDescription* GlobalScope;

		impl::StringCache<const ScopeDescription*> LexicalScopeNameCache;

		Namespace* Parent;
	};


}


