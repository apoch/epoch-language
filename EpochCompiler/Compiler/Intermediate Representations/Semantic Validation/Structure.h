//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a structure definition
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/IdentifierT.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/FunctionSignature.h"
#include "Metadata/CompileTimeParams.h"

#include <map>
#include <vector>


class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Namespace;

	//
	// Abstract base for structure members
	//
	class StructureMember
	{
	// Constants
	public:
		enum Type
		{
			Variable,
			FunctionReference,
		};

	// Destruction
	public:
		virtual ~StructureMember()
		{ }

	// Structure member interface
	public:
		virtual Type GetMemberType() const = 0;
		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const = 0;
		virtual bool Validate(const Namespace& curnamespace, CompileErrors& errors) const = 0;

		virtual StringHandle SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >&, const CompileTimeParameterVector&, Namespace&, CompileErrors&) const = 0;

		virtual void PopulateTypeSpace(Namespace&)
		{ }
	};


	//
	// Structure member variable
	//
	class StructureMemberVariable : public StructureMember
	{
	// Construction
	public:
		StructureMemberVariable(StringHandle type, const AST::IdentifierT& typeidentifier)
			: MyType(type),
			  OriginalTypeName(type),
			  TypeIdentifier(typeidentifier),
			  IsReferenceType(false)
		{ }

	// Non-copyable
	private:
		StructureMemberVariable(const StructureMemberVariable& rhs);
		StructureMemberVariable& operator= (const StructureMemberVariable& rhs);

	// Structure member interface
	public:
		virtual Type GetMemberType() const
		{ return StructureMember::Variable; }

		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;

		virtual bool Validate(const Namespace& curnamespace, CompileErrors& errors) const;

	// Additional queries
	public:
		StringHandle GetNameOfType() const
		{ return MyType; }

		StringHandle GetOriginalNameOfType() const
		{ return OriginalTypeName; }

	// Reference type support
	public:
		void MakeReference()
		{ IsReferenceType = true; }

	// Template support
	public:
		StringHandle SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors) const;

		void SetTemplateArgs(const CompileTimeParameterVector& args)
		{ TemplateArgs = args; }

	// Internal state
	private:
		friend class Structure;

		StringHandle MyType;
		StringHandle OriginalTypeName;
		const AST::IdentifierT& TypeIdentifier;
		CompileTimeParameterVector TemplateArgs;
		bool IsReferenceType;
	};


	//
	// Function reference structure member
	//
	class StructureMemberFunctionReference : public StructureMember
	{
	// Construction
	public:
		explicit StructureMemberFunctionReference(const AST::IdentifierT& functionidentifier)
			: ReturnTypeName(0),
			  ReturnTypeIdentifier(NULL),
			  FunctionIdentifier(functionidentifier)
		{ }

	// Non-copyable
	private:
		StructureMemberFunctionReference(const StructureMemberFunctionReference& rhs);
		StructureMemberFunctionReference& operator= (const StructureMemberFunctionReference& rhs);

	// Structure member interface
	public:
		virtual Type GetMemberType() const
		{ return StructureMember::FunctionReference; }

		virtual Metadata::EpochTypeID GetEpochType(const Namespace& curnamespace) const;

		virtual bool Validate(const Namespace& curnamespace, CompileErrors& errors) const;

		virtual void PopulateTypeSpace(Namespace& curnamespace);

	// Mutation
	public:
		void AddParam(StringHandle type, const AST::IdentifierT* identifier)
		{ ParamTypes.push_back(std::make_pair(type, identifier)); }

		void SetReturnTypeName(StringHandle type, const AST::IdentifierT* identifier)
		{ ReturnTypeName = type; ReturnTypeIdentifier = identifier; }

	// Additional inspection
	public:
		FunctionSignature GetSignature(const Namespace& curnamespace) const;

	// Template support
	public:
		StringHandle SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >&, const CompileTimeParameterVector&, Namespace&, CompileErrors&) const
		{ return 0; }		// TODO

	// Internal state
	private:
		const AST::IdentifierT& FunctionIdentifier;
		StringHandle ReturnTypeName;
		const AST::IdentifierT* ReturnTypeIdentifier;
		std::vector<std::pair<StringHandle, const AST::IdentifierT*> > ParamTypes;
	};


	//
	// Wrapper class for structure definitions
	//
	class Structure
	{
	// Construction and destruction
	public:
		Structure()
			: ConstructorName(0),
			  AnonymousConstructorName(0),
			  CopyConstructorName(0),
			  CompileTimeCodeExecuted(false)
		{ }

		~Structure();

	// Non-copyable
	private:
		Structure(const Structure& other);
		Structure& operator = (const Structure& rhs);

	// Member manipulation
	public:
		void AddMember(StringHandle name, StructureMember* member, CompileErrors& errors);

		const std::vector<std::pair<StringHandle, StructureMember*> >& GetMembers() const
		{ return Members; }

		void SetMemberIsReference(StringHandle name);

	// Template support
	public:
		void AddTemplateParameter(Metadata::EpochTypeID type, StringHandle name);
		void SetMemberTemplateArgs(StringHandle membername, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors);

	// Validation
	public:
		bool Validate(const Namespace& curnamespace, CompileErrors& errors) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(StringHandle myname, Namespace& curnamespace, CompileErrors& errors);
		bool InstantiateTemplate(StringHandle myname, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors);

	// Inspection
	public:
		StringHandle GetConstructorName() const
		{ return ConstructorName; }

		StringHandle GetAnonymousConstructorName() const
		{ return AnonymousConstructorName; }

		StringHandle GetCopyConstructorName() const
		{ return CopyConstructorName; }

		bool IsTemplate() const
		{ return !TemplateParams.empty(); }

		const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& GetTemplateParams() const
		{ return TemplateParams; }

	// Template helpers
	public:
		Metadata::EpochTypeID SubstituteTemplateParams(StringHandle membername, const CompileTimeParameterVector& templateargs, Namespace& curnamespace) const;

	// Internal helpers
	private:
		void GenerateConstructors(StringHandle myname, StringHandle constructorname, StringHandle anonconstructorname, StringHandle copyconstructorname, const CompileTimeParameterVector& templateargs, Namespace& curnamespace, CompileErrors& errors) const;

	// Internal state
	private:
		friend class StructureTable;
		friend class TemplateTable;

		std::vector<std::pair<StringHandle, StructureMember*> > Members;
		StringHandle ConstructorName;
		StringHandle AnonymousConstructorName;
		StringHandle CopyConstructorName;

		std::vector<std::pair<StringHandle, Metadata::EpochTypeID> > TemplateParams;

		bool CompileTimeCodeExecuted;
	};

}

