#pragma once

#include "Parser/SkipGrammar.h"

#include "Compiler/AbstractSyntaxTree.h"


struct ExpressionGrammar;
struct EntityGrammar;


struct CodeBlockGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::CodeBlock()>
{
	typedef std::wstring::const_iterator IteratorT;

	CodeBlockGrammar(const ExpressionGrammar& expressiongrammar, const EntityGrammar& entitygrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};


	Rule<AST::Deferred<AST::CodeBlockEntry>()>::type CodeBlockEntry;
	Rule<AST::CodeBlock()>::type InnerCodeBlock;
	Rule<AST::CodeBlock()>::type CodeBlock;
};

