//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a structure definition
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/FunctionSignature.h"

#include <map>
#include <vector>


class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Program;


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
		virtual VM::EpochTypeID GetEpochType(const IRSemantics::Program& program) const = 0;
		virtual bool Validate(const IRSemantics::Program& program) const = 0;
	};


	//
	// Structure member variable
	//
	class StructureMemberVariable : public StructureMember
	{
	// Construction
	public:
		explicit StructureMemberVariable(StringHandle type)
			: MyType(type)
		{ }

	// Structure member interface
	public:
		virtual Type GetMemberType() const
		{ return StructureMember::Variable; }

		virtual VM::EpochTypeID GetEpochType(const IRSemantics::Program& program) const;

		virtual bool Validate(const IRSemantics::Program& program) const;

	// Internal state
	private:
		StringHandle MyType;
	};


	//
	// Function reference structure member
	//
	class StructureMemberFunctionReference : public StructureMember
	{
	// Construction
	public:
		StructureMemberFunctionReference()
			: ReturnType(VM::EpochType_Void)
		{ }

	// Structure member interface
	public:
		virtual Type GetMemberType() const
		{ return StructureMember::FunctionReference; }

		virtual VM::EpochTypeID GetEpochType(const IRSemantics::Program&) const
		{ return VM::EpochType_Function; }

		virtual bool Validate(const IRSemantics::Program& program) const;

	// Mutation
	public:
		void AddParam(StringHandle type)
		{ ParamTypes.push_back(type); }

		void SetReturnType(StringHandle type)
		{ ReturnType = type; }

	// Additional inspection
	public:
		StringHandle GetReturnType() const
		{ return ReturnType; }

		FunctionSignature GetSignature(const IRSemantics::Program& program) const;

	// Internal state
	private:
		StringHandle ReturnType;
		std::vector<StringHandle> ParamTypes;
	};


	//
	// Wrapper class for structure definitions
	//
	class Structure
	{
	// Construction and destruction
	public:
		Structure()
		{ }

		~Structure();

	// Non-copyable
	private:
		Structure(const Structure& other);
		Structure& operator = (const Structure& rhs);

	// Member manipulation
	public:
		void AddMember(StringHandle name, StructureMember* member);

		const std::vector<std::pair<StringHandle, StructureMember*> >& GetMembers() const
		{ return Members; }

	// Validation
	public:
		bool Validate(const Program& program) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(StringHandle myname, Program& program, CompileErrors& errors);

	// Internal state
	private:
		std::vector<std::pair<StringHandle, StructureMember*> > Members;
	};

}

