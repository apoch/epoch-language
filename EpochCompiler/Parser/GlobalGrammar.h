#pragma once

#include "Compiler/Abstract Syntax Tree/Program.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/TypeDefinitions.h"
#include "Compiler/Abstract Syntax Tree/Templates.h"
#include "Lexer/Lexer.h"

struct FunctionDefinitionGrammar;
struct CodeBlockGrammar;


struct GlobalGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::Program()>
{
	typedef Lexer::TokenIterT IteratorT;

	GlobalGrammar(const Lexer::EpochLexerT& lexer, const FunctionDefinitionGrammar& funcdefgrammar, const CodeBlockGrammar& codeblockgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::MetaEntityVector()>::type MetaEntities;
	Rule<AST::Program()>::type Program;

	Rule<AST::TemplateArgumentList()>::type TemplateArguments;
	Rule<AST::TemplateParameter()>::type TemplateParameter;
	Rule<AST::TemplateParameterList()>::type TemplateParameterList;

	Rule<AST::StructureMember()>::type StructureMember;
	Rule<std::vector<AST::StructureMember, Memory::OneWayAlloc<AST::StructureMember> >()>::type StructureMembers;
	Rule<AST::Deferred<AST::StructureMemberFunctionRef>()>::type StructureMemberFunctionRef;
	Rule<AST::Deferred<AST::StructureMemberVariable>()>::type StructureMemberVariable;

	Rule<AST::DeferredStructure()>::type StructureDefinition;
	Rule<AST::DeferredCodeBlockEntry()>::type GlobalDefinition;
	Rule<AST::MetaEntity()>::type MetaEntity;

	Rule<std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> >()>::type ParamTypeSpec;
	Rule<AST::IdentifierT()>::type ReturnTypeSpec;

	const FunctionDefinitionGrammar& TheFunctionDefinitionGrammar;

	Rule<AST::DeferredTypeAlias()>::type TypeAlias;
	Rule<AST::DeferredStrongTypeAlias()>::type StrongTypeAlias;
	Rule<AST::DeferredSumType()>::type SumType;
	Rule<AST::SumTypeBaseType()>::type SumTypeBaseType;

	Rule<AST::RefTag()>::type RefTagRule;
};

