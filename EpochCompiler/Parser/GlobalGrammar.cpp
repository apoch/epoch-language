// TODO - documentation

#include "pch.h"

#include "Parser/GlobalGrammar.h"
#include "Parser/FunctionDefinitionGrammar.h"

#include "Libraries/Library.h"


GlobalGrammar::GlobalGrammar(const FunctionDefinitionGrammar& funcdefgrammar)
	: GlobalGrammar::base_type(Program),
	  TheFunctionDefinitionGrammar(funcdefgrammar)
{
	using namespace boost::spirit::qi;

	/*InfixIdentifier = L'.';
	PreOperatorStatement = PreOperator >> MemberAccess;
	PostOperatorStatement = MemberAccess >> PostOperator;
	Parenthetical = L'(' >> (PreOperatorStatement | PostOperatorStatement | Expression) >> L')';
	ExpressionComponent = *(UnaryPrefixIdentifier) >> (Statement | Parenthetical | Literal | StringIdentifier);
	Expression = ExpressionComponent >> (*((InfixIdentifier - PreOperator - PostOperator) >> ExpressionComponent));
	Statement = StringIdentifier >> EntityParams;
	ExpressionOrAssignment = Assignment | Expression;
	MemberAccess = StringIdentifier >> *(L'.' >> StringIdentifier);
	Assignment = MemberAccess >> (L'=' | OpAssignmentIdentifier) >> ExpressionOrAssignment;
	AnyStatement = PreOperatorStatement | PostOperatorStatement | Statement;
	CodeBlockEntry = Entity | PostfixEntity | Assignment | AnyStatement | InnerCodeBlock;
	InnerCodeBlock = L'{' >> (*CodeBlockEntry) >> L'}';
	EntityParams = L'(' >> -(Expression % L',') >> L')';
	Entity = EntityIdentifier >> -EntityParams >> InnerCodeBlock >> *ChainedEntity;
	ChainedEntity = ChainedEntityIdentifier >> -EntityParams >> InnerCodeBlock;
	PostfixEntity = PostfixEntityOpenerIdentifier >> -EntityParams >> InnerCodeBlock >> PostfixEntityCloserIdentifier >> L'(' >> Expression >> L')';
	StructureMemberFunctionRef = (StringIdentifier) >> L':' >> ParamTypeSpec >> L"->" >> ReturnTypeSpec;
	StructureMemberVariable = (StringIdentifier >> L'(' >> StringIdentifier >> L')');
	StructureMember = StructureMemberFunctionRef | StructureMemberVariable;
	StructureDefinition = L"structure" >> StringIdentifier >> char_(L':') >> L'(' >> (StructureMember % L',') >> L')';
	GlobalDefinition = L"global" >> CodeBlock;
	FunctionTagSpec = (StringIdentifier >> -(L'(' >> ((Literal | StringIdentifier) % L',') >> L')'));
	FunctionTagList = L"[" >> *FunctionTagSpec >> L"]";*/
	MetaEntity %= /*StructureDefinition | GlobalDefinition |*/ TheFunctionDefinitionGrammar;
	Program %= *MetaEntity;
}
