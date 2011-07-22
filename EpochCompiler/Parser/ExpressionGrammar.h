#pragma once

#include "Parser/OperatorGrammar.h"

#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"
#include "Compiler/Abstract Syntax Tree/AnyStatement.h"

#include "Lexer/Lexer.h"

struct LiteralGrammar;


struct ExpressionGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> >()>
{
	typedef Lexer::TokenIterT IteratorT;

	ExpressionGrammar(const Lexer::EpochLexerT& lexer, const LiteralGrammar& literalgrammar);


	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};

	Rule<AST::Deferred<AST::Statement, boost::intrusive_ptr<AST::Statement> >()>::type Statement;
	Rule<AST::Deferred<AST::PreOperatorStatement, boost::intrusive_ptr<AST::PreOperatorStatement> >()>::type PreOperatorStatement;
	Rule<AST::Deferred<AST::PostOperatorStatement, boost::intrusive_ptr<AST::PostOperatorStatement> >()>::type PostOperatorStatement;
	
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
	Rule<AST::Deferred<AST::ExpressionFragment, boost::intrusive_ptr<AST::ExpressionFragment> >()>::type ExpressionFragment;
	Rule<AST::Deferred<AST::ExpressionComponent, boost::intrusive_ptr<AST::ExpressionComponent> >()>::type ExpressionComponent;
	Rule<AST::Deferred<AST::Expression, boost::intrusive_ptr<AST::Expression> >()>::type Expression;
	Rule<AST::ExpressionOrAssignment()>::type ExpressionOrAssignment;

	Rule<AST::AnyStatement()>::type AnyStatement;
	
	Rule<std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> >()>::type Prefixes;

	typedef boost::spirit::qi::symbols<wchar_t, AST::IdentifierT> SymbolTable;
	SymbolTable PreOperatorSymbols;
	SymbolTable PostOperatorSymbols;
	SymbolTable PrefixSymbols;
	SymbolTable InfixSymbols;
	SymbolTable OpAssignSymbols;
};

