#include "pch.h"

#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/EntityGrammar.h"

#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"


CodeBlockGrammar::CodeBlockGrammar(const Lexer::EpochLexerT& lexer, const ExpressionGrammar& expressiongrammar, const EntityGrammar& entitygrammar)
	: CodeBlockGrammar::base_type(CodeBlock)
{
	using namespace boost::spirit::qi;

	CodeBlockEntry %= entitygrammar | expressiongrammar.Assignment | expressiongrammar.AnyStatement | InnerCodeBlock;
	
	Empty = lexer.CloseBracket;
	InnerCodeBlock %= lexer.OpenBracket >> (Empty | (+CodeBlockEntry >> lexer.CloseBracket));
	CodeBlock %= -InnerCodeBlock;
}

