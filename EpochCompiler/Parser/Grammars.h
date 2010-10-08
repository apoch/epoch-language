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

#include "Compiler/Exceptions.h"



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
	FundamentalGrammar(SemanticActionInterface& bindings, const InfixTable& infixidentifiers, const std::set<std::wstring>& customentities)
		: Bindings(bindings),
		  InfixIdentifiers(infixidentifiers),
		  CustomEntities(customentities)
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
				NEGATE('-'),

				MAPARROW("->"),
				INTEGER("integer"),
				STRING("string"),
				ASSIGN("="),

				ExpectFunctionBody(0)
		{
			using namespace boost::spirit::classic;

			StringIdentifier
				= lexeme_d
				[
					((alpha_p) >> *(alnum_p | '_'))
				]
				;


			IntegerLiteral
				= (!NEGATE) >> (+(digit_p))
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
				| Expression[RegisterPatternMatchedParameter(self.Bindings)]
				;


			ParameterList
				= OPENPARENS[BeginParameterSet(self.Bindings)] >> (!(ParameterDeclaration % COMMA)) >> CLOSEPARENS[EndParameterSet(self.Bindings)]
				;


			ReturnDeclaration
				= TypeMismatchExceptionGuard
				  (
					VariableType[RegisterReturnType(self.Bindings)] >> OPENPARENS >> StringIdentifier[RegisterReturnName(self.Bindings)] >> COMMA >> Expression[RegisterReturnValue(self.Bindings)] >> CLOSEPARENS
				  )[GeneralExceptionHandler(self.Bindings)]
				;

			ReturnList
				= OPENPARENS[BeginReturnSet(self.Bindings)] >> !ReturnDeclaration >> CLOSEPARENS[EndReturnSet(self.Bindings)]
				;


			Literal
				= IntegerLiteral[StoreIntegerLiteral(self.Bindings)]
				| StringLiteral[StoreStringLiteral(self.Bindings)]
				;

			ExpressionComponent
				= Statement
				| (OPENPARENS[BeginParenthetical(self.Bindings)] >> Expression >> CLOSEPARENS)[EndParenthetical(self.Bindings)]
				| Literal
				| StringIdentifier[StoreString(self.Bindings)]
				;

			Expression
				= (ExpressionComponent >> (*(InfixIdentifier[StoreInfix(self.Bindings)] >> ExpressionComponent[PushInfixParam(self.Bindings)])[CompleteInfix(self.Bindings)]))[FinalizeInfix(self.Bindings)]
				;

			Statement
				= (StringIdentifier[BeginStatement(self.Bindings)] >> OPENPARENS[BeginStatementParams(self.Bindings)] >> (!((Expression[PushStatementParam(self.Bindings)]) % COMMA)) >> CLOSEPARENS[CompleteStatement(self.Bindings)])
				;

			ExpressionOrAssignment
				= (Assignment)
				| (Expression[PushStatementParam(self.Bindings)])
				;

			Assignment
				= (StringIdentifier[BeginStatement(self.Bindings)] >> ASSIGN[BeginAssignment(self.Bindings)] >> ExpressionOrAssignment[CompleteAssignment(self.Bindings)])
				;

			CodeBlockEntry
				= GeneralExceptionGuard
				  (
					TypeMismatchExceptionGuard
					(
						((Entity) | (Assignment) | (Statement[FinalizeStatement(self.Bindings)]) | InnerCodeBlock)
					)[GeneralExceptionHandler(self.Bindings)]
				  )[GeneralExceptionHandler(self.Bindings)]
				;

			InnerCodeBlock
				= OPENBRACE[BeginLexicalScope(self.Bindings)] >> (*CodeBlockEntry) >> CLOSEBRACE[EndLexicalScope(self.Bindings)]
				;

			CodeBlock
				= OPENBRACE[EmitPendingCode(self.Bindings)] >> (*CodeBlockEntry) >> CLOSEBRACE
				;

			Entity
				= (EntityIdentifier[BeginEntityChain(self.Bindings)][StoreEntityTypeByString(self.Bindings)][BeginEntityParams(self.Bindings)]
				  >> !(OPENPARENS >> (Expression[PushStatementParam(self.Bindings)] % COMMA) >> CLOSEPARENS)
				  >> OPENBRACE[CompleteEntityParams(self.Bindings)] >> (*CodeBlockEntry) >> CLOSEBRACE[EndLexicalScope(self.Bindings)]
				  >> *ChainedEntity)[EndEntityChain(self.Bindings)]
				;

			ChainedEntity
				= (ChainedEntityIdentifier[StoreEntityTypeByString(self.Bindings)][BeginEntityParams(self.Bindings)]
				  >> !(OPENPARENS >> (Expression[PushStatementParam(self.Bindings)] % COMMA) >> CLOSEPARENS)
				  >> OPENBRACE[CompleteEntityParams(self.Bindings)] >> (*CodeBlockEntry) >> CLOSEBRACE[EndLexicalScope(self.Bindings)])
				;

			MetaEntity
				= StringIdentifier[StoreString(self.Bindings)] >> COLON >> ParameterList >> MAPARROW >> ReturnList[StoreEntityType(self.Bindings, Bytecode::EntityTags::Function)]
					>> MissingFunctionBodyExceptionGuard(ExpectFunctionBody(CodeBlock[StoreEntityCode(self.Bindings)]))[MissingFunctionBodyExceptionHandler(self.Bindings)]
				;

			Program
				= (*MetaEntity)[Finalize(self.Bindings)]
				;

			for(InfixTable::const_iterator iter = self.InfixIdentifiers.begin(); iter != self.InfixIdentifiers.end(); ++iter)
				AddInfixOperator(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(std::set<std::wstring>::const_iterator iter = self.CustomEntities.begin(); iter != self.CustomEntities.end(); ++iter)
				AddInlineEntity(*PooledNarrowStrings.insert(narrow(*iter)).first);
		}

		boost::spirit::classic::chlit<> COLON, OPENPARENS, CLOSEPARENS, OPENBRACE, CLOSEBRACE, COMMA, QUOTE, NEGATE;

		boost::spirit::classic::strlit<> MAPARROW, INTEGER, STRING, ASSIGN;

		boost::spirit::classic::rule<ScannerType> StringIdentifier;

		boost::spirit::classic::rule<ScannerType> StringLiteral;
		boost::spirit::classic::rule<ScannerType> IntegerLiteral;
		boost::spirit::classic::rule<ScannerType> Literal;

		boost::spirit::classic::rule<ScannerType> ExpressionComponent;
		boost::spirit::classic::rule<ScannerType> Expression;
		boost::spirit::classic::rule<ScannerType> ExpressionOrAssignment;
		boost::spirit::classic::rule<ScannerType> Statement;
		boost::spirit::classic::rule<ScannerType> Assignment;

		boost::spirit::classic::rule<ScannerType> VariableType;

		boost::spirit::classic::rule<ScannerType> ParameterDeclaration;
		boost::spirit::classic::rule<ScannerType> ReturnDeclaration;

		boost::spirit::classic::rule<ScannerType> ParameterList;
		boost::spirit::classic::rule<ScannerType> ReturnList;
		
		boost::spirit::classic::rule<ScannerType> CodeBlockEntry;
		boost::spirit::classic::rule<ScannerType> CodeBlock;
		boost::spirit::classic::rule<ScannerType> InnerCodeBlock;

		boost::spirit::classic::rule<ScannerType> MetaEntity;
		boost::spirit::classic::rule<ScannerType> Entity;
		boost::spirit::classic::rule<ScannerType> ChainedEntity;

		boost::spirit::classic::rule<ScannerType> Program;

		boost::spirit::classic::stored_rule<ScannerType> InfixIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> EntityIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> ChainedEntityIdentifier;

		boost::spirit::classic::guard<RecoverableException> GeneralExceptionGuard;
		boost::spirit::classic::guard<TypeMismatchException> TypeMismatchExceptionGuard;

		boost::spirit::classic::guard<int> MissingFunctionBodyExceptionGuard;
		boost::spirit::classic::assertion<int> ExpectFunctionBody;
		
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

		//
		// Register an inline entity
		//
		void AddInlineEntity(const std::string& entityname)
		{
			EntityIdentifier = boost::spirit::classic::strlit<>(entityname.c_str()) | EntityIdentifier.copy();
		}
	};

	SemanticActionInterface& Bindings;
	const InfixTable& InfixIdentifiers;
	const std::set<std::wstring>& CustomEntities;

};

