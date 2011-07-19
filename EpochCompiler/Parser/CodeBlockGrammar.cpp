#include "pch.h"

#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/EntityGrammar.h"


CodeBlockGrammar::CodeBlockGrammar(const Lexer::EpochLexerT& lexer, const ExpressionGrammar& expressiongrammar, const EntityGrammar& entitygrammar)
	: CodeBlockGrammar::base_type(CodeBlock)
{
	using namespace boost::spirit::qi;

	CodeBlockEntry %= entitygrammar | expressiongrammar.Assignment | expressiongrammar.AnyStatement | InnerCodeBlock;
	
	InnerCodeBlock %= lexer.OpenBracket >> (*CodeBlockEntry) >> lexer.CloseBracket;
	CodeBlock %= -InnerCodeBlock;
}

