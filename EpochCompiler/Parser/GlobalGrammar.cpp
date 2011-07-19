// TODO - documentation

#include "pch.h"

#include "Parser/GlobalGrammar.h"
#include "Parser/FunctionDefinitionGrammar.h"
#include "Parser/UtilityGrammar.h"
#include "Parser/CodeBlockGrammar.h"

#include "Libraries/Library.h"


GlobalGrammar::GlobalGrammar(const Lexer::EpochLexerT& lexer, const FunctionDefinitionGrammar& funcdefgrammar, const UtilityGrammar& identifiergrammar, const CodeBlockGrammar& codeblockgrammar)
	: GlobalGrammar::base_type(Program),
	  TheFunctionDefinitionGrammar(funcdefgrammar)
{
	using namespace boost::spirit::qi;

	ParamTypeSpec %= lexer.OpenParens >> -(identifiergrammar % lexer.Comma) >> lexer.CloseParens;
	ReturnTypeSpec %= lexer.OpenParens >> -(identifiergrammar) >> lexer.CloseParens;
	StructureMemberFunctionRef %= (identifiergrammar) >> lexer.Colon >> ParamTypeSpec >> lexer.Arrow >> ReturnTypeSpec;
	StructureMemberVariable %= (identifiergrammar >> lexer.OpenParens >> identifiergrammar >> lexer.CloseParens);
	StructureMember %= StructureMemberFunctionRef | StructureMemberVariable;
	StructureDefinition %= lexer.StructureDef >> identifiergrammar >> lexer.Colon >> lexer.OpenParens >> (StructureMember % lexer.Comma) >> lexer.CloseParens;
	GlobalDefinition %= lexer.GlobalDef >> codeblockgrammar;
	MetaEntity %= StructureDefinition | GlobalDefinition | TheFunctionDefinitionGrammar;
	Program %= *MetaEntity;
}
