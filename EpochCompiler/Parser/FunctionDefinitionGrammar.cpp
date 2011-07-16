#include "pch.h"
#include "Parser/FunctionDefinitionGrammar.h"

#include "Parser/UtilityGrammar.h"
#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"


FunctionDefinitionGrammar::FunctionDefinitionGrammar(const CodeBlockGrammar& codeblockgrammar, const UtilityGrammar& identifiergrammar, const ExpressionGrammar& expressiongrammar)
	: FunctionDefinitionGrammar::base_type(FunctionDefinition)
{
	using namespace boost::spirit::qi;

	ParamTypeSpec = L'(' >> -(identifiergrammar % L',') >> L')';
	ReturnTypeSpec = raw[L'(' >> -(raw[identifiergrammar]) >> L')'];
	ParameterFunctionRef = identifiergrammar >> L':' >> ParamTypeSpec >> L"->" >> ReturnTypeSpec;
	ParameterSpec %= identifiergrammar >> -lit(L"ref") >> L'(' >> identifiergrammar >> L')';
	ParameterDeclaration %= ParameterFunctionRef | ParameterSpec | expressiongrammar;
	ParameterList %= L'(' >> (-(ParameterDeclaration % L',')) >> L')';
	ReturnDeclaration %= expressiongrammar;
	ReturnList %= L'(' >> -ReturnDeclaration >> L')';

	FunctionTagSpec = (identifiergrammar >> -(L'(' >> ((expressiongrammar) % L',') >> L')'));
	FunctionTagList = L"[" >> *FunctionTagSpec >> L"]";

	FunctionDefinition %= identifiergrammar >> L':' >> ParameterList >> L"->" >> ReturnList >> -FunctionTagList >> codeblockgrammar;
}
