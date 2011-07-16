#include "pch.h"

#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/EntityGrammar.h"


CodeBlockGrammar::CodeBlockGrammar(const ExpressionGrammar& expressiongrammar, const EntityGrammar& entitygrammar)
	: CodeBlockGrammar::base_type(CodeBlock)
{
	using namespace boost::spirit::qi;

	CodeBlockEntry %= entitygrammar | expressiongrammar.Assignment | expressiongrammar.AnyStatement | InnerCodeBlock;
	
	InnerCodeBlock %= L'{' >> (*CodeBlockEntry) >> L'}';
	CodeBlock %= -InnerCodeBlock;
}

