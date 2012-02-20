// TODO - documentation

#include "pch.h"

#include "Parser/GlobalGrammar.h"
#include "Parser/FunctionDefinitionGrammar.h"
#include "Parser/CodeBlockGrammar.h"

#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"

#include "Libraries/Library.h"


GlobalGrammar::GlobalGrammar(const Lexer::EpochLexerT& lexer, const FunctionDefinitionGrammar& funcdefgrammar, const CodeBlockGrammar& codeblockgrammar)
	: GlobalGrammar::base_type(Program),
	  TheFunctionDefinitionGrammar(funcdefgrammar)
{
	using namespace boost::spirit::qi;

	ParamTypeSpec %= lexer.OpenParens >> -(as<AST::IdentifierT>()[lexer.StringIdentifier] % lexer.Comma) >> lexer.CloseParens;
	ReturnTypeSpec %= lexer.OpenParens >> -(lexer.StringIdentifier) >> lexer.CloseParens;
	StructureMemberFunctionRef %= (lexer.StringIdentifier) >> lexer.Colon >> ParamTypeSpec >> lexer.Arrow >> ReturnTypeSpec;
	StructureMemberVariable %= (lexer.StringIdentifier >> lexer.OpenParens >> lexer.StringIdentifier >> lexer.CloseParens);
	StructureMember %= StructureMemberVariable | StructureMemberFunctionRef;
	StructureMembers %= (StructureMember % lexer.Comma);
	StructureDefinition %= lexer.StructureDef >> lexer.StringIdentifier >> lexer.Colon >> lexer.OpenParens >> StructureMembers >> lexer.CloseParens;
	GlobalDefinition %= lexer.GlobalDef >> codeblockgrammar.InnerCodeBlock;
	MetaEntity %= GlobalDefinition | StructureDefinition | TheFunctionDefinitionGrammar;
	MetaEntities %= *MetaEntity;
	Program %= MetaEntities;
}
