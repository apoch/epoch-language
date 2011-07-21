//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// boost::spirit grammars for parsing the fundamental elements of the Epoch language
//

#pragma once


// Dependencies
#include "Parser/SemanticActionBindings.h"

#include "Bytecode/EntityTags.h"

#include "Libraries/Library.h"

#include "Compiler/AbstractSyntaxTree.h"
#include "Compiler/Exceptions.h"

#include "Lexer/Lexer.h"

#include "Parser/LiteralGrammar.h"
#include "Parser/GlobalGrammar.h"
#include "Parser/FunctionDefinitionGrammar.h"
#include "Parser/CodeBlockGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/EntityGrammar.h"


struct FundamentalGrammar : public boost::spirit::qi::grammar<Lexer::TokenIterT, boost::spirit::char_encoding::standard_wide, AST::Program()>
{
	typedef Lexer::TokenIterT IteratorT;

	FundamentalGrammar(Lexer::EpochLexerT& lexer, const IdentifierTable& identifiers)
		: FundamentalGrammar::base_type(StartRule),
		  TheLexer(lexer),
		  TheLiteralGrammar(lexer),
		  TheExpressionGrammar(lexer, TheLiteralGrammar),
		  TheCodeBlockGrammar(lexer, TheExpressionGrammar, TheEntityGrammar),
		  TheFunctionDefinitionGrammar(lexer, TheCodeBlockGrammar, TheExpressionGrammar),
		  TheGlobalGrammar(lexer, TheFunctionDefinitionGrammar, TheCodeBlockGrammar),
		  Identifiers(identifiers)
	{
		StartRule %= TheGlobalGrammar;

		//
		// Set up dynamic parser rules to handle all the custom identifiers and symbols
		// that might be needed to parse the given program, e.g. operators, entities,
		// and so on. This permits programs and external libraries to create their own
		// operators, entities, etc. etc. for extending the language itself.
		//
		for(StringSet::const_iterator iter = Identifiers.InfixOperators.begin(); iter != Identifiers.InfixOperators.end(); ++iter)
			AddInfixOperator(*iter);

		for(StringSet::const_iterator iter = Identifiers.UnaryPrefixes.begin(); iter != Identifiers.UnaryPrefixes.end(); ++iter)
			AddUnaryPrefix(*iter);

		for(StringSet::const_iterator iter = Identifiers.PreOperators.begin(); iter != Identifiers.PreOperators.end(); ++iter)
			AddPreOperator(*iter);

		for(StringSet::const_iterator iter = Identifiers.PostOperators.begin(); iter != Identifiers.PostOperators.end(); ++iter)
			AddPostOperator(*iter);

		for(StringSet::const_iterator iter = Identifiers.CustomEntities.begin(); iter != Identifiers.CustomEntities.end(); ++iter)
			AddInlineEntity(*iter);

		for(StringSet::const_iterator iter = Identifiers.ChainedEntities.begin(); iter != Identifiers.ChainedEntities.end(); ++iter)
			AddChainedEntity(*iter);

		for(StringSet::const_iterator iter = Identifiers.PostfixEntities.begin(); iter != Identifiers.PostfixEntities.end(); ++iter)
			AddPostfixEntity(*iter);

		for(StringSet::const_iterator iter = Identifiers.PostfixClosers.begin(); iter != Identifiers.PostfixClosers.end(); ++iter)
			AddPostfixCloser(*iter);

		for(StringSet::const_iterator iter = Identifiers.OpAssignmentIdentifiers.begin(); iter != Identifiers.OpAssignmentIdentifiers.end(); ++iter)
			AddOpAssignOperator(*iter);

		TheEntityGrammar.InitRecursivePortion(lexer, TheExpressionGrammar, TheCodeBlockGrammar);
	}

	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, AttributeT> type;
	};
	
	Rule<AST::Program()>::type StartRule;

	Lexer::EpochLexerT& TheLexer;

	LiteralGrammar TheLiteralGrammar;
	ExpressionGrammar TheExpressionGrammar;
	EntityGrammar TheEntityGrammar;
	CodeBlockGrammar TheCodeBlockGrammar;
	FunctionDefinitionGrammar TheFunctionDefinitionGrammar;
	GlobalGrammar TheGlobalGrammar;


	//
	// Register an infix operator function
	//
	void AddInfixOperator(const std::wstring& opname)
	{
		TheExpressionGrammar.InfixSymbols.add(opname);
	}

	//
	// Register a unary prefix operator
	//
	void AddUnaryPrefix(const std::wstring& opname)
	{
		TheExpressionGrammar.PrefixSymbols.add(opname);
	}

	//
	// Register a pre-operator
	//
	void AddPreOperator(const std::wstring& opname)
	{
		TheExpressionGrammar.PreOperatorSymbols.add(opname);
	}

	//
	// Register a post-operator
	//
	void AddPostOperator(const std::wstring& opname)
	{
		TheExpressionGrammar.PostOperatorSymbols.add(opname);
	}

	//
	// Register an inline entity
	//
	void AddInlineEntity(const std::wstring& entityname)
	{
		TheEntityGrammar.EntityIdentifierSymbols.add(entityname);
	}

	//
	// Register a chained entity
	//
	void AddChainedEntity(const std::wstring& entityname)
	{
		TheEntityGrammar.ChainedEntityIdentifierSymbols.add(entityname);
	}

	//
	// Register a postfix entity
	//
	void AddPostfixEntity(const std::wstring& entityname)
	{
		TheEntityGrammar.PostfixEntitySymbols.add(entityname);
	}

	//
	// Register a postfix entity closer
	//
	void AddPostfixCloser(const std::wstring& entityname)
	{
		TheEntityGrammar.PostfixEntityCloserSymbols.add(entityname);
	}

	//
	// Register an op-assign operator
	//
	void AddOpAssignOperator(const std::wstring& identifier)
	{
		TheExpressionGrammar.OpAssignSymbols.add(identifier);
	}

	const IdentifierTable& Identifiers;

};

