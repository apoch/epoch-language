#pragma once

#include "Compiler/Abstract Syntax Tree/AbstractSyntaxTree.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
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

	Rule<AST::StructureMember()>::type StructureMember;
	Rule<std::vector<AST::StructureMember, Memory::OneWayAlloc<AST::StructureMember> >()>::type StructureMembers;
	Rule<AST::Deferred<AST::StructureMemberFunctionRef>()>::type StructureMemberFunctionRef;
	Rule<AST::Deferred<AST::StructureMemberVariable>()>::type StructureMemberVariable;

	Rule<AST::Deferred<AST::Structure>()>::type StructureDefinition;
	Rule<AST::DeferredCodeBlock()>::type GlobalDefinition;
	Rule<AST::MetaEntity()>::type MetaEntity;

	Rule<std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> >()>::type ParamTypeSpec;
	Rule<AST::IdentifierT()>::type ReturnTypeSpec;

	const FunctionDefinitionGrammar& TheFunctionDefinitionGrammar;
};