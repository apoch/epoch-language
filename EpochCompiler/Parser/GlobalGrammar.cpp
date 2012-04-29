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

	ParamTypeSpec %= -(as<AST::IdentifierT>()[lexer.StringIdentifier] % lexer.Comma);
	ReturnTypeSpec %= lexer.Arrow >> lexer.StringIdentifier;
	StructureMemberFunctionRef %= lexer.OpenParens >> (lexer.StringIdentifier) >> lexer.Colon >> ParamTypeSpec >> -ReturnTypeSpec >> lexer.CloseParens;
	StructureMemberVariable %= (lexer.StringIdentifier >> lexer.StringIdentifier);
	StructureMember %= StructureMemberVariable | StructureMemberFunctionRef;
	StructureMembers %= (StructureMember % lexer.Comma);
	StructureDefinition %= lexer.StructureDef >> lexer.StringIdentifier >> lexer.Colon >> StructureMembers;
	GlobalDefinition %= lexer.GlobalDef >> codeblockgrammar.InnerCodeBlock;
	TypeAlias = lexer.AliasDef >> lexer.StringIdentifier >> omit[lexer.Equals] >> lexer.StringIdentifier;
	StrongTypeAlias = lexer.TypeDef >> lexer.StringIdentifier >> lexer.Colon >> lexer.StringIdentifier;
	SumType = lexer.TypeDef >> lexer.StringIdentifier >> lexer.Colon >> ((lexer.StringIdentifier | as<AST::IdentifierT>()[lexer.Nothing]) % lexer.Pipe);
	MetaEntity %= GlobalDefinition | StructureDefinition | TheFunctionDefinitionGrammar | TypeAlias | SumType | StrongTypeAlias;
	MetaEntities %= *MetaEntity;
	Program %= MetaEntities;
}
