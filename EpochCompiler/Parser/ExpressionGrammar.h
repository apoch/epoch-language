#pragma once

#include "Parser/SkipGrammar.h"
#include "Parser/OperatorGrammar.h"

#include "Compiler/AbstractSyntaxTree.h"

struct LiteralGrammar;
struct UtilityGrammar;


struct ExpressionGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::Deferred<AST::Expression>()>
{
	typedef std::wstring::const_iterator IteratorT;

	ExpressionGrammar(const LiteralGrammar& literalgrammar, const UtilityGrammar& identifiergrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};

	Rule<AST::Statement()>::type Statement;
	Rule<AST::Deferred<AST::PreOperatorStatement>()>::type PreOperatorStatement;
	Rule<AST::Deferred<AST::PostOperatorStatement>()>::type PostOperatorStatement;
	
	Rule<AST::Assignment()>::type Assignment;
	Rule<AST::IdentifierT()>::type AssignmentOperator;

	Rule<AST::Parenthetical()>::type Parenthetical;
	Rule<std::list<AST::Deferred<AST::Expression> >()>::type EntityParams;
	Rule<AST::MemberAccess()>::type MemberAccess;

	Rule<AST::Deferred<AST::ExpressionFragment>()>::type ExpressionFragment;
	Rule<AST::Deferred<AST::ExpressionComponent>()>::type ExpressionComponent;
	Rule<AST::Deferred<AST::Expression>()>::type Expression;
	Rule<AST::ExpressionOrAssignment()>::type ExpressionOrAssignment;

	Rule<AST::AnyStatement()>::type AnyStatement;
	
	Rule<std::list<AST::IdentifierT>()>::type Prefixes;

	SymbolTable PreOperator;
	SymbolTable PostOperator;
	SymbolTable OpAssignmentIdentifier;
	SymbolTable InfixIdentifier;
	SymbolTable UnaryPrefixIdentifier;
};

