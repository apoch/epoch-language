//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a structure definition
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"


using namespace IRSemantics;


//
// Destruct and clean up a structure definition wrapper
//
Structure::~Structure()
{
	for(std::vector<std::pair<StringHandle, StructureMember*> >::iterator iter = Members.begin(); iter != Members.end(); ++iter)
		delete iter->second;
}

//
// Add a member to a structure definition
//
void Structure::AddMember(StringHandle name, StructureMember* member)
{
	for(std::vector<std::pair<StringHandle, StructureMember*> >::iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(name == iter->first)
		{
			delete member;
			throw std::exception("Duplicate structure member");		// TODO - better exceptions
		}
	}

	Members.push_back(std::make_pair(name, member));
}

//
// Validate a structure definition
//
bool Structure::Validate(const Program& program) const
{
	bool valid = true;

	for(std::vector<std::pair<StringHandle, StructureMember*> >::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
	{
		if(!iter->second->Validate(program))
			valid = false;
	}

	return valid;
}


//
// Validate a variable structure member
//
bool StructureMemberVariable::Validate(const Program& program) const
{
	return (program.LookupType(MyType) != VM::EpochType_Error);
}


VM::EpochTypeID StructureMemberVariable::GetEpochType(const Program& program) const
{
	return program.LookupType(MyType);
}



//
// Validate a function reference structure member
//
bool StructureMemberFunctionReference::Validate(const Program& program) const
{
	bool valid = true;

	if(program.LookupType(ReturnType) == VM::EpochType_Error)
		valid = false;

	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		if(program.LookupType(*iter) == VM::EpochType_Error)
			valid = false;
	}
	
	return valid;
}

