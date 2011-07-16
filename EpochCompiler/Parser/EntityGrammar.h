#pragma once

#include "Parser/SkipGrammar.h"

#include "Compiler/AbstractSyntaxTree.h"


struct ExpressionGrammar;
struct CodeBlockGrammar;


struct EntityGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::AnyEntity()>
{
	typedef std::wstring::const_iterator IteratorT;

	EntityGrammar();

	void InitRecursivePortion(const ExpressionGrammar& expressiongrammar, const CodeBlockGrammar& codeblockgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};

	// TODO - this is redundant with OperatorGrammar.h :: SymbolTable
	typedef boost::spirit::qi::symbols<wchar_t, AST::IdentifierT> Symbols;

	Rule<AST::Entity()>::type Entity;
	Rule<AST::ChainedEntity()>::type ChainedEntity;
	Rule<AST::PostfixEntity()>::type PostfixEntity;

	Rule<AST::AnyEntity()>::type AnyEntity;

	Symbols EntityIdentifier;
	Symbols ChainedEntityIdentifier;
	Symbols PostfixEntityOpenerIdentifier;
	Symbols PostfixEntityCloserIdentifier;
};

