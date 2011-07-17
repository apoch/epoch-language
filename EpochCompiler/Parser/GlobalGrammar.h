#pragma once

#include "Parser/SkipGrammar.h"
#include "Compiler/AbstractSyntaxTree.h"

struct FunctionDefinitionGrammar;
struct UtilityGrammar;
struct CodeBlockGrammar;


struct GlobalGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::Program()>
{
	typedef std::wstring::const_iterator IteratorT;

	GlobalGrammar(const FunctionDefinitionGrammar& funcdefgrammar, const UtilityGrammar& identifiergrammar, const CodeBlockGrammar& codeblockgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
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