#include "pch.h"
#include "Parser/FunctionDefinitionGrammar.h"

#include "Parser/UtilityGrammar.h"
#include "Parser/CodeBlockGrammar.h"


FunctionDefinitionGrammar::FunctionDefinitionGrammar(const CodeBlockGrammar& codeblockgrammar, const UtilityGrammar& identifiergrammar)
	: FunctionDefinitionGrammar::base_type(FunctionDefinition)
{
	using namespace boost::spirit::qi;

	ParamTypeSpec = L'(' >> -(identifiergrammar % L',') >> L')';
	ReturnTypeSpec = L'(' >> -(identifiergrammar) >> L')';
	ParameterFunctionRef = identifiergrammar >> L':' >> ParamTypeSpec >> L"->" >> ReturnTypeSpec;
	ParameterSpec %= identifiergrammar >> /*-lit(L"ref") >>*/ L'(' >> identifiergrammar >> L')';
	ParameterDeclaration %= ParameterFunctionRef | ParameterSpec;// | Expression;
	ParameterList %= L'(' >> (-(ParameterDeclaration % L',')) >> L')';
	//ReturnDeclaration = Expression.alias();
	ReturnList %= L'(' >> /*-ReturnDeclaration >>*/ char_(L')');

	FunctionDefinition %= identifiergrammar >> L':' >> ParameterList >> L"->" >> ReturnList >> /*-FunctionTagList >> */codeblockgrammar;
}
