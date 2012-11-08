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
		virtual VM::EpochTypeID GetEpochType(const Namespace& curnamespace) const = 0;
		virtual bool Validate(const Namespace& curnamespace, CompileErrors& errors) const = 0;
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
			  TypeIdentifier(typeidentifier)
		{ }

	// Non-copyable
	private:
		StructureMemberVariable(const StructureMemberVariable& rhs);
		StructureMemberVariable& operator= (const StructureMemberVariable& rhs);

	// Structure member interface
	public:
		virtual Type GetMemberType() const
		{ return StructureMember::Variable; }

		virtual VM::EpochTypeID GetEpochType(const Namespace& curnamespace) const;

		virtual bool Validate(const Namespace& curnamespace, CompileErrors& errors) const;

	// Additional queries
	public:
		StringHandle GetNameOfType() const
		{ return MyType; }

	// Internal state
	private:
		StringHandle MyType;
		const AST::IdentifierT& TypeIdentifier;
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

		virtual VM::EpochTypeID GetEpochType(const Namespace&) const
		{ return VM::EpochType_Function; }

		virtual bool Validate(const Namespace& curnamespace, CompileErrors& errors) const;

	// Mutation
	public:
		void AddParam(StringHandle type, const AST::IdentifierT* identifier)
		{ ParamTypes.push_back(std::make_pair(type, identifier)); }

		void SetReturnTypeName(StringHandle type, const AST::IdentifierT* identifier)
		{ ReturnTypeName = type; ReturnTypeIdentifier = identifier; }

	// Additional inspection
	public:
		FunctionSignature GetSignature(const Namespace& curnamespace) const;

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
			: ConstructorName(0)
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

	// Template support
	public:
		void AddTemplateParameter(VM::EpochTypeID type, StringHandle name);

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

		bool IsTemplate() const
		{ return !TemplateParams.empty(); }

	// Template helpers
	public:
		VM::EpochTypeID SubstituteTemplateParams(StringHandle membername, const CompileTimeParameterVector& templateargs, const Namespace& curnamespace) const;

	// Internal helpers
	private:
		void GenerateConstructors(StringHandle myname, StringHandle constructorname, StringHandle anonconstructorname, const CompileTimeParameterVector& templateargs, Namespace& curnamespace, CompileErrors& errors) const;

	// Internal state
	private:
		std::vector<std::pair<StringHandle, StructureMember*> > Members;
		StringHandle ConstructorName;
		StringHandle AnonymousConstructorName;

		std::vector<std::pair<StringHandle, VM::EpochTypeID> > TemplateParams;
	};

}

