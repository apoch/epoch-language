// TODO - documentation

#include "pch.h"

#include "Parser/GlobalGrammar.h"
#include "Parser/FunctionDefinitionGrammar.h"
#include "Parser/UtilityGrammar.h"
#include "Parser/CodeBlockGrammar.h"

#include "Libraries/Library.h"


GlobalGrammar::GlobalGrammar(const FunctionDefinitionGrammar& funcdefgrammar, const UtilityGrammar& identifiergrammar, const CodeBlockGrammar& codeblockgrammar)
	: GlobalGrammar::base_type(Program),
	  TheFunctionDefinitionGrammar(funcdefgrammar)
{
	using namespace boost::spirit::qi;

	ParamTypeSpec %= L'(' >> -(raw[identifiergrammar] % L',') >> L')';
	ReturnTypeSpec %= raw[L'(' >> -(raw[identifiergrammar]) >> L')'];
	StructureMemberFunctionRef %= (raw[identifiergrammar]) >> L':' >> ParamTypeSpec >> L"->" >> ReturnTypeSpec;
	StructureMemberVariable %= (raw[identifiergrammar] >> L'(' >> raw[identifiergrammar] >> L')');
	StructureMember %= StructureMemberFunctionRef | StructureMemberVariable;
	StructureDefinition %= L"structure" >> raw[identifiergrammar] >> lit(L":") >> L'(' >> (StructureMember % L',') >> L')';
	GlobalDefinition %= L"global" >> codeblockgrammar;
	MetaEntity %= StructureDefinition | GlobalDefinition | TheFunctionDefinitionGrammar;
	Program %= *MetaEntity;
}
