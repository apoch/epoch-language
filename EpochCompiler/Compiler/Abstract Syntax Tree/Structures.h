//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for structure definitions
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/OptionalIdentifier.h"
#include "Compiler/Abstract Syntax Tree/Templates.h"


namespace AST
{

	//
	// A simple structure member variable
	//
	struct StructureMemberVariable
	{
		IdentifierT Type;
		OptionalTemplateArgumentList TemplateArgs;
		IdentifierT Name;
	};

	//
	// A function reference member variable
	//
	struct StructureMemberFunctionRef
	{
		IdentifierT Name;
		IdentifierList ParamTypes;
		OptionalIdentifier ReturnType;
	};


	//
	// Variant including any legal structure member
	//
	typedef boost::variant
		<
			Undefined,
			Deferred<StructureMemberVariable>,
			Deferred<StructureMemberFunctionRef>
		> StructureMember;


	//
	// AST node representing a complete structure definition
	//
	struct Structure
	{
		IdentifierT Identifier;
		OptionalTemplateParameterList TemplateParams;
		
		std::vector<StructureMember, Memory::OneWayAlloc<StructureMember> > Members;

		long RefCount;

		Structure()
			: RefCount(0)
		{ }

	// Non-copyable
	private:
		Structure(const Structure&);
		Structure& operator = (const Structure&);
	};


}

//
// Adapters for treating our AST node structures as boost::fusion sequences
//

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::StructureMemberFunctionRef>,
	(AST::IdentifierT, Content->Name)
	(AST::IdentifierList, Content->ParamTypes)
	(AST::OptionalIdentifier, Content->ReturnType)
)

BOOST_FUSION_ADAPT_STRUCT
(
	AST::Deferred<AST::StructureMemberVariable>,
	(AST::IdentifierT, Content->Type)
	(AST::OptionalTemplateArgumentList, Content->TemplateArgs)
	(AST::IdentifierT, Content->Name)
)

typedef std::vector<AST::StructureMember, Memory::OneWayAlloc<AST::StructureMember> > StructureMemberVec;

BOOST_FUSION_ADAPT_STRUCT
(
	AST::DeferredStructure,
	(AST::IdentifierT, Content->Identifier)
	(AST::OptionalTemplateParameterList, Content->TemplateParams)
	(StructureMemberVec, Content->Members)
)
