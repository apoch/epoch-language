#pragma once

#include "Parser/OperatorGrammar.h"

#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"
#include "Compiler/Abstract Syntax Tree/AnyStatement.h"

#include "Lexer/Lexer.h"

struct LiteralGrammar;


struct ExpressionGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::DeferredExpression()>
{
	typedef Lexer::TokenIterT IteratorT;

	ExpressionGrammar(const Lexer::EpochLexerT& lexer, const LiteralGrammar& literalgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::DeferredStatement()>::type Statement;
	Rule<AST::DeferredPreOperatorStatement()>::type PreOperatorStatement;
	Rule<AST::DeferredPostOperatorStatement()>::type PostOperatorStatement;

	Rule<AST::TemplateArgumentList()>::type TemplateArguments;
	Rule<AST::DeferredInitialization()>::type VariableDeclaration;
	
	Rule<AST::DeferredSimpleAssignment()>::type SimpleAssignment;
	Rule<AST::DeferredAssignment()>::type MemberAssignment;
	Rule<AST::DeferredAssignment()>::type Assignment;
	Rule<AST::IdentifierT()>::type AssignmentOperator;

	Rule<AST::Parenthetical()>::type Parenthetical;
	Rule<boost::spirit::qi::unused_type>::type EntityParamsEmpty;
	Rule<AST::DeferredExpressionVector()>::type EntityParamsInner;
	Rule<AST::DeferredExpressionVector()>::type EntityParams;
	Rule<AST::IdentifierList()>::type MemberAccess;

	Rule<AST::Deferred<AST::ExpressionComponentInternal, boost::intrusive_ptr<AST::ExpressionComponentInternal> >()>::type ExpressionChunk;
	Rule<AST::DeferredExpressionFragment()>::type ExpressionFragment;
	Rule<AST::DeferredExpressionComponent()>::type ExpressionComponent;
	Rule<AST::DeferredExpression()>::type Expression;
	Rule<AST::ExpressionOrAssignment()>::type ExpressionOrAssignment;

	Rule<AST::AnyStatement()>::type AnyStatement;
	
	Rule<AST::IdentifierT()>::type Prefix;
	Rule<std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> >()>::type Prefixes;

	typedef boost::spirit::qi::symbols<wchar_t, AST::IdentifierT> SymbolTable;
	SymbolTable PreOperatorSymbols;
	SymbolTable PostOperatorSymbols;
	SymbolTable PrefixSymbols;
	SymbolTable InfixSymbols;
	SymbolTable OpAssignSymbols;
};

