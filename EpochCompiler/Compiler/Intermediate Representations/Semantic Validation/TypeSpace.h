//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR management objects for type metadata and other tracking
//

#pragma once


// Dependencies
#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/ExportDef.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/CompileTimeParams.h"


// Forward declarations
class CompileErrors;
class ScopeDescription;



namespace IRSemantics
{

	// Forward declarations
	class Structure;
	class TypeSpace;
	class Namespace;


	// Handy type shortcuts
	typedef std::map<StringHandle, CompileTimeParameterVector> InstancesAndArguments;
	typedef std::map<StringHandle, InstancesAndArguments> InstantiationMap;

	//
	// Helper class for managing type IDs
	//
	// This must be global across an entire program since the VM will use
	// the same numeric ID space for all executing code.
	//
	class GlobalIDSpace
	{
	// Construction
	public:
		GlobalIDSpace();

	// Non-copyable
	private:
		GlobalIDSpace(const GlobalIDSpace&);
		GlobalIDSpace& operator = (const GlobalIDSpace&);

	// ID allocation interface
	public:
		Metadata::EpochTypeID NewStructureTypeID();
		Metadata::EpochTypeID NewUnitTypeID();
		Metadata::EpochTypeID NewSumTypeID();
		Metadata::EpochTypeID NewTemplateInstantiation();
		Metadata::EpochTypeID NewFunctionSignature();

	// Internal tracking
	private:
		unsigned CounterStructureTypeIDs;
		unsigned CounterUnitTypeIDs;
		unsigned CounterSumTypeIDs;
		unsigned CounterTemplateInstantiations;
		unsigned CounterFunctionSignatures;
	};


	//
	// Wrapper class for a table of structure type definitions
	//
	class StructureTable
	{
	// Construction and destruction
	public:
		explicit StructureTable(TypeSpace& typespace);
		~StructureTable();

	// Non-copyable
	private:
		StructureTable(const StructureTable&);
		StructureTable& operator = (const StructureTable&);

	// Table manipulation interface
	public:
		void Add(StringHandle name, Structure* structure, CompileErrors& errors);

	// Table query interface
	public:
		const Structure* GetDefinition(StringHandle structurename) const;
		Structure* GetDefinition(StringHandle structurename);

		const std::map<StringHandle, Structure*>& GetDefinitions() const
		{ return NameToDefinitionMap; }

		StringHandle GetConstructorName(StringHandle structurename) const;
		Metadata::EpochTypeID GetMemberType(StringHandle structurename, StringHandle membername) const;
		size_t GetMemberOffset(StringHandle structurename, StringHandle membername) const;

		bool IsStructureTemplate(StringHandle name) const;

	// Internal tracking
	private:
		friend class TypeSpace;
		friend class TemplateTable;

		TypeSpace& MyTypeSpace;

		std::map<StringHandle, Structure*> NameToDefinitionMap;
		std::map<StringHandle, Metadata::EpochTypeID> NameToTypeMap;
	};


	//
	// Wrapper class for a table of algebraic sum type definitions
	//
	class SumTypeTable
	{
	// Construction
	public:
		explicit SumTypeTable(TypeSpace& typespace);

	// Non-copyable
	private:
		SumTypeTable(const SumTypeTable&);
		SumTypeTable& operator = (const SumTypeTable&);

	// Table manipulation interface
	public:
		Metadata::EpochTypeID Add(const std::wstring& name, CompileErrors& errors);
		void AddBaseTypeToSumType(Metadata::EpochTypeID sumtypeid, StringHandle basetypename);

	// Table query interface
	public:
		bool IsBaseType(Metadata::EpochTypeID sumtypeid, Metadata::EpochTypeID basetype) const;
		size_t GetNumBaseTypes(Metadata::EpochTypeID sumtypeid) const;

		StringHandle MapConstructorName(StringHandle sumtypeoverloadname) const;

		std::map<Metadata::EpochTypeID, std::set<Metadata::EpochTypeID> > GetDefinitions() const;

	// Templated sum type support
	public:
		void AddTemplateParameter(Metadata::EpochTypeID sumtype, StringHandle name);

		bool IsTemplate(StringHandle name) const;
		StringHandle InstantiateTemplate(StringHandle templatename, const CompileTimeParameterVector& args, CompileErrors& errors);

		void GenerateCode();

	// Internal helpers
	private:
		std::wstring GenerateTemplateMangledName(Metadata::EpochTypeID templatetype, Metadata::EpochTypeID instancetype, const CompileTimeParameterVector& args);

	// Internal tracking
	private:
		friend class TypeSpace;
		friend class FunctionTable;
		TypeSpace& MyTypeSpace;

		std::map<StringHandle, Metadata::EpochTypeID> NameToTypeMap;
		std::map<StringHandle, StringHandle> NameToConstructorMap;
		std::map<Metadata::EpochTypeID, std::set<StringHandle> > BaseTypeNames;

		InstantiationMap Instantiations;
		std::map<StringHandle, std::vector<StringHandle> > NameToParamsMap;

		std::set<Metadata::EpochTypeID> PendingCodeGen;
	};


	//
	// Wrapper class for a table of structure template definitions
	//
	class TemplateTable
	{
	// Construction
	public:
		explicit TemplateTable(TypeSpace& typespace);

	// Non-copyable
	private:
		TemplateTable(const TemplateTable&);
		TemplateTable& operator = (const TemplateTable&);

	// Structure templates
	public:
		StringHandle InstantiateStructure(StringHandle templatename, const CompileTimeParameterVector& args, CompileErrors& errors);

		const InstantiationMap& GetInstantiations() const
		{ return Instantiations; }

		StringHandle GetTemplateForInstance(StringHandle instancename) const;

	// Constructor helpers
	public:
		StringHandle FindConstructorName(StringHandle instancename) const;
		StringHandle FindAnonConstructorName(StringHandle instancename) const;

	// Internal helpers
	private:
		std::wstring GenerateTemplateMangledName(Metadata::EpochTypeID templatetype, Metadata::EpochTypeID instancetype, const CompileTimeParameterVector& args);

	// Internal tracking
	private:
		friend class TypeSpace;
		friend class StructureTable;

		TypeSpace& MyTypeSpace;

		InstantiationMap Instantiations;
		std::map<StringHandle, Metadata::EpochTypeID> NameToTypeMap;

		impl::StringCache<StringHandle> ConstructorNameCache;
		impl::StringCache<StringHandle> AnonConstructorNameCache;
	};


	//
	// Wrapper class for tables of type aliases (both weak and strong)
	//
	class TypeAliasTable
	{
	// Construction
	public:
		explicit TypeAliasTable(TypeSpace& typespace);

	// Non-copyable
	private:
		TypeAliasTable(const TypeAliasTable&);
		TypeAliasTable& operator = (const TypeAliasTable&);

	// Weak type alias management interface
	public:
		void AddWeakAlias(StringHandle aliasname, Metadata::EpochTypeID representationtype);
		bool HasWeakAliasNamed(StringHandle name) const;
		StringHandle GetWeakTypeBaseName(StringHandle name) const;

	// Strong type alias management interface
	public:
		void AddStrongAlias(StringHandle aliasname, Metadata::EpochTypeID representationtype, StringHandle representationname);

		Metadata::EpochTypeID GetStrongRepresentation(Metadata::EpochTypeID aliastypeid) const;
		StringHandle GetStrongRepresentationName(Metadata::EpochTypeID aliastypeid) const;

		void CacheStrongAliasSizes(std::map<Metadata::EpochTypeID, size_t>& sizecache) const;

		const std::map<Metadata::EpochTypeID, Metadata::EpochTypeID>& GetStrongAliasMap() const;

	// Internal tracking
	private:
		friend class TypeSpace;
		TypeSpace& MyTypeSpace;

		std::map<StringHandle, Metadata::EpochTypeID> WeakNameToTypeMap;
		std::map<StringHandle, Metadata::EpochTypeID> StrongNameToTypeMap;
		std::map<Metadata::EpochTypeID, Metadata::EpochTypeID> StrongRepresentationTypes;
		std::map<Metadata::EpochTypeID, StringHandle> StrongRepresentationNames;
	};


	//
	// Wrapper class for managing higher-order function signatures
	//
	class FunctionSignatureTable
	{
	// Construction
	public:
		explicit FunctionSignatureTable(TypeSpace& typespace);

	// Non-copyable
	private:
		FunctionSignatureTable(const FunctionSignatureTable&);
		FunctionSignatureTable& operator = (const FunctionSignatureTable&);

	// Population interface
	public:
		Metadata::EpochTypeID AllocateEpochType(const FunctionSignature& sig);

	// Lookup interface
	public:
		Metadata::EpochTypeID FindEpochType(const FunctionSignature& sig) const;

		const std::vector<std::pair<FunctionSignature, Metadata::EpochTypeID> >& GetDefinitions() const
		{ return SignatureToTypeList; }

	// Internal state
	private:
		TypeSpace& MyTypeSpace;
		std::vector<std::pair<FunctionSignature, Metadata::EpochTypeID> > SignatureToTypeList;
	};


	//
	// Wrapper class for a typespace
	//
	class TypeSpace
	{
	// Construction
	public:
		TypeSpace(Namespace& mynamespace, GlobalIDSpace& idspace);

	// Non-copyable
	private:
		TypeSpace(const TypeSpace&);
		TypeSpace& operator = (const TypeSpace&);

	// Name lookup interface
	public:
		EPOCHCOMPILER Metadata::EpochTypeID GetTypeByName(StringHandle name) const;
		StringHandle GetNameOfType(Metadata::EpochTypeID type) const;

	// Compiler internals
	public:
		bool Validate(CompileErrors& errors) const;
		bool CompileTimeCodeExecution(CompileErrors& errors);

	// Components
	public:
		TypeAliasTable Aliases;
		StructureTable Structures;
		SumTypeTable SumTypes;
		TemplateTable Templates;
		FunctionSignatureTable FunctionSignatures;

	// Internal tracking
	private:
		friend class StructureTable;
		friend class TemplateTable;
		friend class TypeAliasTable;
		friend class SumTypeTable;
		friend class FunctionSignatureTable;
		friend class Namespace;

		Namespace& MyNamespace;
		GlobalIDSpace& IDSpace;
	};

}
