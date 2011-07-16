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

#include "Parser/SkipGrammar.h"
#include "Parser/LiteralGrammar.h"
#include "Parser/GlobalGrammar.h"
#include "Parser/FunctionDefinitionGrammar.h"
#include "Parser/CodeBlockGrammar.h"
#include "Parser/UtilityGrammar.h"
#include "Parser/ExpressionGrammar.h"
#include "Parser/EntityGrammar.h"


struct FundamentalGrammar : public boost::spirit::qi::grammar<std::wstring::const_iterator, boost::spirit::char_encoding::standard_wide, SkipGrammar, AST::Program()>
{
	typedef std::wstring::const_iterator IteratorT;

	explicit FundamentalGrammar(const IdentifierTable& identifiers)
		: FundamentalGrammar::base_type(StartRule),
		  TheExpressionGrammar(TheLiteralGrammar, TheIdentifierGrammar),
		  TheCodeBlockGrammar(TheExpressionGrammar, TheEntityGrammar),
		  TheFunctionDefinitionGrammar(TheCodeBlockGrammar, TheIdentifierGrammar, TheExpressionGrammar),
		  TheGlobalGrammar(TheFunctionDefinitionGrammar, TheIdentifierGrammar, TheCodeBlockGrammar),
		  Identifiers(identifiers)
	{
		StartRule %= TheGlobalGrammar;

		TheEntityGrammar.InitRecursivePortion(TheExpressionGrammar, TheCodeBlockGrammar);

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
	}

	template <typename AttributeT>
	struct Rule
	{
		typedef typename boost::spirit::qi::rule<IteratorT, boost::spirit::char_encoding::standard_wide, SkipGrammar, AttributeT> type;
	};
	
	Rule<AST::Program()>::type StartRule;

	UtilityGrammar TheIdentifierGrammar;
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
		TheExpressionGrammar.InfixIdentifier.add(opname.c_str());
	}

	//
	// Register a unary prefix operator
	//
	void AddUnaryPrefix(const std::wstring& opname)
	{
		TheExpressionGrammar.UnaryPrefixIdentifier.add(opname.c_str());
	}

	//
	// Register a pre-operator
	//
	void AddPreOperator(const std::wstring& opname)
	{
		TheExpressionGrammar.PreOperator.add(opname.c_str());
	}

	//
	// Register a post-operator
	//
	void AddPostOperator(const std::wstring& opname)
	{
		TheExpressionGrammar.PostOperator.add(opname.c_str());
	}

	//
	// Register an inline entity
	//
	void AddInlineEntity(const std::wstring& entityname)
	{
		TheEntityGrammar.EntityIdentifier.add(entityname, entityname);
	}

	//
	// Register a chained entity
	//
	void AddChainedEntity(const std::wstring& entityname)
	{
		TheEntityGrammar.ChainedEntityIdentifier.add(entityname.c_str());
	}

	//
	// Register a postfix entity
	//
	void AddPostfixEntity(const std::wstring& entityname)
	{
		TheEntityGrammar.PostfixEntityOpenerIdentifier.add(entityname);
	}

	//
	// Register a postfix entity closer
	//
	void AddPostfixCloser(const std::wstring& entityname)
	{
		TheEntityGrammar.PostfixEntityCloserIdentifier.add(entityname);
	}

	//
	// Register an op-assign operator
	//
	void AddOpAssignOperator(const std::wstring& identifier)
	{
		TheExpressionGrammar.OpAssignmentIdentifier.add(identifier);
	}

	const IdentifierTable& Identifiers;

};

