#pragma once

#include "Compiler/AbstractSyntaxTree.h"
#include "Lexer/Lexer.h"

struct FunctionDefinitionGrammar;
struct UtilityGrammar;
struct CodeBlockGrammar;


struct GlobalGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::Program()>
{
	typedef Lexer::TokenIterT IteratorT;

	GlobalGrammar(const Lexer::EpochLexerT& lexer, const FunctionDefinitionGrammar& funcdefgrammar, const UtilityGrammar& identifiergrammar, const CodeBlockGrammar& codeblockgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::Program()>::type Program;

	Rule<AST::StructureMember()>::type StructureMember;
	Rule<AST::StructureMemberFunctionRef()>::type StructureMemberFunctionRef;
	Rule<AST::StructureMemberVariable()>::type StructureMemberVariable;

	Rule<AST::Structure()>::type StructureDefinition;
	Rule<AST::CodeBlock()>::type GlobalDefinition;
	Rule<AST::MetaEntity()>::type MetaEntity;

	Rule<std::vector<AST::IdentifierT>()>::type ParamTypeSpec;
	Rule<AST::IdentifierT()>::type ReturnTypeSpec;

	const FunctionDefinitionGrammar& TheFunctionDefinitionGrammar;
};