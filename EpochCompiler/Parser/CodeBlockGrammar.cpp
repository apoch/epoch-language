#include "pch.h"
#include "Parser/CodeBlockGrammar.h"


CodeBlockGrammar::CodeBlockGrammar()
	: CodeBlockGrammar::base_type(CodeBlock)
{
	using namespace boost::spirit::qi;

	CodeBlock %= -(char_(L'{') >> /*(*CodeBlockEntry) >> */char_(L'}'));
}