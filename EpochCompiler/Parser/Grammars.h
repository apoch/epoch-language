//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// boost::spirit grammars for parsing the fundamental elements of the Epoch language
//

#pragma once


// Dependencies
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_stored_rule.hpp>

#include "Parser/SemanticActionBindings.h"

#include "Bytecode/EntityTags.h"

#include "Libraries/Library.h"



//
// Grammar for defining what to skip during the parse phase.
// We ignore whitespace and code comments.
//
struct SkipGrammar : public boost::spirit::classic::grammar<SkipGrammar>
{
	template <typename ScannerType>
	struct definition
	{
		definition(const SkipGrammar&)
		{
			using namespace boost::spirit::classic;

			Skip
				= space_p
				| '\r'
				| '\n'
				| '\t'
				| comment_p("//")
				;
		}

		boost::spirit::classic::rule<ScannerType> Skip;

		//
		// Retrieve the parser's starting (root) symbol
		//
		const boost::spirit::classic::rule<ScannerType>& start() const
		{ return Skip; }
	};
};


//
// Grammar that actually parses the raw source code and invokes
// the appropriate semantic actions for the client.
//
struct FundamentalGrammar : public boost::spirit::classic::grammar<FundamentalGrammar>
{
	FundamentalGrammar(SemanticActionInterface& bindings, const InfixTable& infixidentifiers)
		: Bindings(bindings),
		  InfixIdentifiers(infixidentifiers)
	{ }

	template <typename ScannerType>
	struct definition
	{
		definition(const FundamentalGrammar& self)
			:
				COLON(':'),
				OPENPARENS('('),
				CLOSEPARENS(')'),
				OPENBRACE('{'),
				CLOSEBRACE('}'),
				COMMA(','),
				QUOTE('\"'),

				MAPARROW("->"),
				INTEGER("integer"),
				STRING("string")
		{
			using namespace boost::spirit::classic;

			StringIdentifier
				= lexeme_d
				[
					((alpha_p) >> *(alnum_p | '_'))
				]
				;


			IntegerLiteral
				= (+(digit_p))
				;

			StringLiteral
				= QUOTE >> *(anychar_p - QUOTE) >> QUOTE
				;


			VariableType
				= INTEGER
				| STRING
				;

			
			ParameterDeclaration
				= VariableType[RegisterParameterType(self.Bindings)] >> OPENPARENS >> StringIdentifier[RegisterParameterName(self.Bindings)] >> CLOSEPARENS
				;


			ParameterList
				= OPENPARENS[BeginParameterSet(self.Bindings)] >> (!(ParameterDeclaration % COMMA)) >> CLOSEPARENS[EndParameterSet(self.Bindings)]
				;

			ReturnList
				= OPENPARENS >> CLOSEPARENS
				;


			Literal
				= IntegerLiteral[StoreIntegerLiteral(self.Bindings)]
				| StringLiteral[StoreStringLiteral(self.Bindings)]
				;

			ExpressionComponent
				= Statement
				| Literal
				| StringIdentifier[StoreString(self.Bindings)]
				;

			Expression
				= (ExpressionComponent >> *(InfixIdentifier[StoreInfix(self.Bindings)] >> ExpressionComponent[ValidateStatementParam(self.Bindings)])[CompleteInfix(self.Bindings)])
				;

			Statement
				= StringIdentifier[BeginStatement(self.Bindings)] >> OPENPARENS[BeginStatementParams(self.Bindings)] >> (!(Expression[ValidateStatementParam(self.Bindings)] % COMMA)) >> CLOSEPARENS[CompleteStatement(self.Bindings)]
				;

			CodeBlock
				= OPENBRACE >> (*Statement) >> CLOSEBRACE
				;

			MetaEntity
				= StringIdentifier[StoreString(self.Bindings)] >> COLON >> ParameterList >> MAPARROW >> ReturnList[StoreEntityType(self.Bindings, Bytecode::EntityTags::Function)] >> CodeBlock[StoreEntityCode(self.Bindings)];

			Program
				= (*MetaEntity)[Finalize(self.Bindings)]
				;

			for(InfixTable::const_iterator iter = self.InfixIdentifiers.begin(); iter != self.InfixIdentifiers.end(); ++iter)
				AddInfixOperator(*PooledNarrowStrings.insert(narrow(*iter)).first);
		}

		boost::spirit::classic::chlit<> COLON, OPENPARENS, CLOSEPARENS, OPENBRACE, CLOSEBRACE, COMMA, QUOTE;

		boost::spirit::classic::strlit<> MAPARROW, INTEGER, STRING;

		boost::spirit::classic::rule<ScannerType> StringIdentifier;

		boost::spirit::classic::rule<ScannerType> StringLiteral;
		boost::spirit::classic::rule<ScannerType> IntegerLiteral;
		boost::spirit::classic::rule<ScannerType> Literal;

		boost::spirit::classic::rule<ScannerType> ExpressionComponent;
		boost::spirit::classic::rule<ScannerType> Expression;
		boost::spirit::classic::rule<ScannerType> Statement;

		boost::spirit::classic::rule<ScannerType> VariableType;

		boost::spirit::classic::rule<ScannerType> ParameterDeclaration;

		boost::spirit::classic::rule<ScannerType> ParameterList;
		boost::spirit::classic::rule<ScannerType> ReturnList;
		boost::spirit::classic::rule<ScannerType> CodeBlock;

		boost::spirit::classic::rule<ScannerType> MetaEntity;

		boost::spirit::classic::rule<ScannerType> Program;

		boost::spirit::classic::stored_rule<ScannerType> InfixIdentifier;
		
		std::set<std::string> PooledNarrowStrings;

		//
		// Retrieve the parser's starting (root) symbol
		//
		const boost::spirit::classic::rule<ScannerType>& start() const
		{ return Program; }

		//
		// Register an infix operator function
		//
		void AddInfixOperator(const std::string& opname)
		{
			InfixIdentifier = boost::spirit::classic::strlit<>(opname.c_str()) | InfixIdentifier.copy();
		}
	};

	SemanticActionInterface& Bindings;
	const InfixTable& InfixIdentifiers;

};

