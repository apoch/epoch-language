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
		VM::EpochTypeID NewStructureTypeID();
		VM::EpochTypeID NewUnitTypeID();
		VM::EpochTypeID NewSumTypeID();
		VM::EpochTypeID NewTemplateInstantiation();

	// Internal tracking
	private:
		unsigned CounterStructureTypeIDs;
		unsigned CounterUnitTypeIDs;
		unsigned CounterSumTypeIDs;
		unsigned CounterTemplateInstantiations;
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
		const std::map<StringHandle, Structure*>& GetDefinitions() const
		{ return NameToDefinitionMap; }

		StringHandle GetConstructorName(StringHandle structurename) const;
		VM::EpochTypeID GetMemberType(StringHandle structurename, StringHandle membername) const;

	// Internal tracking
	private:
		friend class TypeSpace;
		TypeSpace& MyTypeSpace;

		std::map<StringHandle, Structure*> NameToDefinitionMap;
		std::map<StringHandle, VM::EpochTypeID> NameToTypeMap;
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
		VM::EpochTypeID Add(const std::wstring& name, CompileErrors& errors);
		void AddBaseTypeToSumType(VM::EpochTypeID sumtypeid, StringHandle basetypename);

	// Table query interface
	public:
		bool IsBaseType(VM::EpochTypeID sumtypeid, VM::EpochTypeID basetype) const;
		size_t GetNumBaseTypes(VM::EpochTypeID sumtypeid) const;

		StringHandle MapConstructorName(StringHandle sumtypeoverloadname) const;

		std::map<VM::EpochTypeID, std::set<VM::EpochTypeID> > GetDefinitions() const;

	// Internal tracking
	private:
		friend class TypeSpace;
		TypeSpace& MyTypeSpace;

		std::map<StringHandle, VM::EpochTypeID> NameToTypeMap;
		std::map<StringHandle, StringHandle> NameToConstructorMap;
		std::map<VM::EpochTypeID, std::set<StringHandle> > BaseTypeNames;
	};


	//
	// Wrapper class for a table of structure template definitions
	//
	class TemplateTable
	{
	// Handy type shortcuts
	public:
		typedef std::map<StringHandle, CompileTimeParameterVector> InstancesAndArguments;
		typedef std::map<StringHandle, InstancesAndArguments> InstantiationMap;

	// Construction
	public:
		explicit TemplateTable(TypeSpace& typespace);

	// Non-copyable
	private:
		TemplateTable(const TemplateTable&);
		TemplateTable& operator = (const TemplateTable&);

	// Structure templates
	public:
		StringHandle InstantiateStructure(StringHandle templatename, const CompileTimeParameterVector& args);

		const InstantiationMap& GetInstantiations() const
		{ return Instantiations; }

	// Constructor helpers
	public:
		StringHandle FindConstructorName(StringHandle instancename) const;
		StringHandle FindAnonConstructorName(StringHandle instancename) const;

	// Internal helpers
	private:
		static std::wstring GenerateTemplateMangledName(VM::EpochTypeID type);

	// Internal tracking
	private:
		friend class TypeSpace;
		TypeSpace& MyTypeSpace;

		InstantiationMap Instantiations;
		std::map<StringHandle, VM::EpochTypeID> NameToTypeMap;

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
		void AddWeakAlias(StringHandle aliasname, VM::EpochTypeID representationtype);

	// Strong type alias management interface
	public:
		void AddStrongAlias(StringHandle aliasname, VM::EpochTypeID representationtype, StringHandle representationname);

		VM::EpochTypeID GetStrongRepresentation(VM::EpochTypeID aliastypeid) const;
		StringHandle GetStrongRepresentationName(VM::EpochTypeID aliastypeid) const;

	// Internal tracking
	private:
		friend class TypeSpace;
		TypeSpace& MyTypeSpace;

		std::map<StringHandle, VM::EpochTypeID> WeakNameToTypeMap;
		std::map<StringHandle, VM::EpochTypeID> StrongNameToTypeMap;
		std::map<VM::EpochTypeID, VM::EpochTypeID> StrongRepresentationTypes;
		std::map<VM::EpochTypeID, StringHandle> StrongRepresentationNames;
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
		EPOCHCOMPILER VM::EpochTypeID GetTypeByName(StringHandle name) const;
		StringHandle GetNameOfType(VM::EpochTypeID type) const;

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

	// Internal tracking
	private:
		friend class StructureTable;
		friend class TemplateTable;
		friend class TypeAliasTable;
		friend class SumTypeTable;

		Namespace& MyNamespace;
		GlobalIDSpace& IDSpace;
	};

}
