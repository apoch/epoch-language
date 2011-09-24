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

#include <map>
#include <vector>


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
		StructureMemberFunctionReference(const std::vector<StringHandle>& paramtypes, StringHandle returntype)
			: ReturnType(returntype),
			  ParamTypes(paramtypes)
		{ }

	// Structure member interface
	public:
		virtual Type GetMemberType() const
		{ return StructureMember::FunctionReference; }

		virtual VM::EpochTypeID GetEpochType(const IRSemantics::Program& program) const
		{ return VM::EpochType_Function; }

		virtual bool Validate(const IRSemantics::Program& program) const;

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
		bool Validate(const IRSemantics::Program& program) const;

	// Internal state
	private:
		std::vector<std::pair<StringHandle, StructureMember*> > Members;
	};

}

