#pragma once

#include "Compiler/Abstract Syntax Tree/Identifiers.h"

namespace AST
{

	struct StructureMemberVariable
	{
		IdentifierT Type;
		IdentifierT Name;
	};

	struct StructureMemberFunctionRef
	{
		IdentifierT Name;
		IdentifierList ParamTypes;
		IdentifierT ReturnType;
	};


	typedef boost::variant
		<
			Undefined,
			Deferred<StructureMemberVariable>,
			Deferred<StructureMemberFunctionRef>
		> StructureMember;


	struct Structure
	{
		IdentifierT Identifier;
		std::vector<StructureMember, Memory::OneWayAlloc<StructureMember> > Members;
	};


}

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::StructureMemberFunctionRef>,
	(AST::IdentifierT, Content->Name)
	(AST::IdentifierList, Content->ParamTypes)
	(AST::IdentifierT, Content->ReturnType)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::StructureMemberVariable>,
	(AST::IdentifierT, Content->Type)
	(AST::IdentifierT, Content->Name)
)

typedef std::vector<AST::StructureMember, Memory::OneWayAlloc<AST::StructureMember> > StructureMemberVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::Structure>,
	(AST::IdentifierT, Content->Identifier)
	(StructureMemberVec, Content->Members)
)
