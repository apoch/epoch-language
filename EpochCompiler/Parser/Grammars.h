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
	FundamentalGrammar(SemanticActionInterface& bindings, const IdentifierTable& identifiers)
		: Bindings(bindings),
		  Identifiers(identifiers)
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
				PERIOD('.'),
				COMMA(','),
				QUOTE('\"'),
				NEGATE('-'),

				MAPARROW("->"),
				
				INTEGER("integer"),
				STRING("string"),
				BOOLEAN("boolean"),
				REAL("real"),

				ASSIGN("="),

				EPOCH_TRUE("true"),
				EPOCH_FALSE("false"),

				ExpectFunctionBody(0),

				ExpectWellFormedStatement(MalformedStatementException("Expected a statement or code control entity"))
		{
			using namespace boost::spirit::classic;

			StringIdentifier
				= lexeme_d
				[
					((alpha_p) >> *(alnum_p | '_'))
				]
				;

			RealLiteral
				= (!NEGATE) >> (+(digit_p)) >> PERIOD >> (+(digit_p))
				;

			IntegerLiteral
				= (!NEGATE) >> (+(digit_p))
				;

			StringLiteral
				= QUOTE >> *(anychar_p - QUOTE) >> QUOTE
				;

			BooleanLiteral
				= EPOCH_TRUE
				| EPOCH_FALSE
				;


			VariableType
				= INTEGER
				| STRING
				| BOOLEAN
				| REAL
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
				= RealLiteral[StoreRealLiteral(self.Bindings)]
				| IntegerLiteral[StoreIntegerLiteral(self.Bindings)]
				| StringLiteral[StoreStringLiteral(self.Bindings)]
				| BooleanLiteral[StoreBooleanLiteral(self.Bindings)]
				;

			ExpressionComponent
				= *(UnaryPrefixIdentifier[StoreUnaryPrefixOperator(self.Bindings)])
				>> (
					Statement
					| (OPENPARENS[BeginParenthetical(self.Bindings)] >> Expression >> CLOSEPARENS)[EndParenthetical(self.Bindings)]
					| Literal
					| PreOperatorStatement
					| PostOperatorStatement
					| StringIdentifier[StoreString(self.Bindings)]
				   )
				;

			Expression
				= (ExpressionComponent >> (*(InfixIdentifier[StoreInfix(self.Bindings)] >> ExpressionComponent[PushInfixParam(self.Bindings)])[CompleteInfix(self.Bindings)]))[FinalizeInfix(self.Bindings)]
				;

			Statement
				= (StringIdentifier[BeginStatement(self.Bindings)] >> OPENPARENS[BeginStatementParams(self.Bindings)] >> (!((Expression[PushStatementParam(self.Bindings)]) % COMMA)) >> CLOSEPARENS[CompleteStatement(self.Bindings)])
				;

			PreOperatorStatement
				= PreOperator[RegisterPreOperator(self.Bindings)] >> StringIdentifier[RegisterPreOperand(self.Bindings)]
				;

			PostOperatorStatement
				= StringIdentifier[RegisterPostOperand(self.Bindings)] >> PostOperator[RegisterPostOperator(self.Bindings)]
				;

			ExpressionOrAssignment
				= (Assignment)
				| (Expression[PushStatementParam(self.Bindings)])
				;

			Assignment
				= (StringIdentifier[BeginStatement(self.Bindings)]
				>> (ASSIGN[BeginAssignment(self.Bindings)] | OpAssignmentIdentifier[BeginOpAssignment(self.Bindings)])
				>> ExpressionOrAssignment[CompleteAssignment(self.Bindings)])
				;

			CodeBlockEntry
				= GeneralExceptionGuard
				  (
					TypeMismatchExceptionGuard
					(
						MalformedStatementExceptionGuard
						(
							ExpectWellFormedStatement((Entity) | (PostfixEntity) | (Assignment) | ((PreOperatorStatement) | (PostOperatorStatement) | (Statement))[FinalizeStatement(self.Bindings)] | InnerCodeBlock)
						)[MalformedStatementExceptionHandler(self.Bindings)]
					)[GeneralExceptionHandler(self.Bindings)]
				  )[GeneralExceptionHandler(self.Bindings)]
				;

			InnerCodeBlock
				= OPENBRACE[BeginLexicalScope(self.Bindings)] >> (*CodeBlockEntry) >> CLOSEBRACE[EndLexicalScope(self.Bindings)]
				;

			CodeBlock
				= (OPENBRACE[EmitPendingCode(self.Bindings)] >> (*CodeBlockEntry) >> CLOSEBRACE)
				;

			Entity
				= (EntityIdentifier[BeginEntityChain(self.Bindings)][StoreEntityTypeByString(self.Bindings)][BeginEntityParams(self.Bindings)]
				  >> !(OPENPARENS >> (Expression[PushStatementParam(self.Bindings)] % COMMA) >> CLOSEPARENS)
				  >> OPENBRACE[CompleteEntityParams(self.Bindings, false)] >> (*CodeBlockEntry) >> CLOSEBRACE[EndLexicalScope(self.Bindings)]
				  >> *ChainedEntity)[EndEntityChain(self.Bindings)]
				;

			ChainedEntity
				= (ChainedEntityIdentifier[StoreEntityTypeByString(self.Bindings)][BeginEntityParams(self.Bindings)]
				  >> !(OPENPARENS >> (Expression[PushStatementParam(self.Bindings)] % COMMA) >> CLOSEPARENS)
				  >> OPENBRACE[CompleteEntityParams(self.Bindings, false)] >> (*CodeBlockEntry) >> CLOSEBRACE[EndLexicalScope(self.Bindings)])
				;

			PostfixEntity
				= (PostfixEntityOpenerIdentifier[BeginEntityChain(self.Bindings)][StoreEntityTypeByString(self.Bindings)][BeginEntityParams(self.Bindings)]
				  >> !(OPENPARENS >> (Expression[PushStatementParam(self.Bindings)] % COMMA) >> CLOSEPARENS)
				  >> OPENBRACE[CompleteEntityParams(self.Bindings, false)] >> (*CodeBlockEntry) >> CLOSEBRACE
				  >> PostfixEntityCloserIdentifier[StoreEntityPostfixByString(self.Bindings)][BeginEntityParams(self.Bindings)]
				  >> OPENPARENS >> Expression[PushStatementParam(self.Bindings)] >> CLOSEPARENS[CompleteEntityParams(self.Bindings, true)][InvokePostfixMetacontrol(self.Bindings)])[EndEntityChain(self.Bindings)]
				;

			MetaEntity
				= StringIdentifier[StoreString(self.Bindings)] >> COLON >> ParameterList >> MAPARROW >> ReturnList[StoreEntityType(self.Bindings, Bytecode::EntityTags::Function)]
					>> MissingFunctionBodyExceptionGuard(ExpectFunctionBody(CodeBlock[StoreEntityCode(self.Bindings)]))[MissingFunctionBodyExceptionHandler(self.Bindings)]
				;

			Program
				= (*MetaEntity)[Finalize(self.Bindings)]
				;

			for(StringSet::const_iterator iter = self.Identifiers.InfixOperators.begin(); iter != self.Identifiers.InfixOperators.end(); ++iter)
				AddInfixOperator(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.UnaryPrefixes.begin(); iter != self.Identifiers.UnaryPrefixes.end(); ++iter)
				AddUnaryPrefix(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.PreOperators.begin(); iter != self.Identifiers.PreOperators.end(); ++iter)
				AddPreOperator(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.PostOperators.begin(); iter != self.Identifiers.PostOperators.end(); ++iter)
				AddPostOperator(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.CustomEntities.begin(); iter != self.Identifiers.CustomEntities.end(); ++iter)
				AddInlineEntity(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.ChainedEntities.begin(); iter != self.Identifiers.ChainedEntities.end(); ++iter)
				AddChainedEntity(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.PostfixEntities.begin(); iter != self.Identifiers.PostfixEntities.end(); ++iter)
				AddPostfixEntity(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.PostfixClosers.begin(); iter != self.Identifiers.PostfixClosers.end(); ++iter)
				AddPostfixCloser(*PooledNarrowStrings.insert(narrow(*iter)).first);

			for(StringSet::const_iterator iter = self.Identifiers.OpAssignmentIdentifiers.begin(); iter != self.Identifiers.OpAssignmentIdentifiers.end(); ++iter)
				AddOpAssignOperator(*PooledNarrowStrings.insert(narrow(*iter)).first);
		}

		boost::spirit::classic::chlit<> COLON, OPENPARENS, CLOSEPARENS, OPENBRACE, CLOSEBRACE, PERIOD, COMMA, QUOTE, NEGATE;

		boost::spirit::classic::strlit<> MAPARROW, INTEGER, STRING, BOOLEAN, REAL, ASSIGN, EPOCH_TRUE, EPOCH_FALSE;

		boost::spirit::classic::rule<ScannerType> StringIdentifier;

		boost::spirit::classic::rule<ScannerType> StringLiteral;
		boost::spirit::classic::rule<ScannerType> IntegerLiteral;
		boost::spirit::classic::rule<ScannerType> BooleanLiteral;
		boost::spirit::classic::rule<ScannerType> RealLiteral;
		boost::spirit::classic::rule<ScannerType> Literal;

		boost::spirit::classic::rule<ScannerType> ExpressionComponent;
		boost::spirit::classic::rule<ScannerType> Expression;
		boost::spirit::classic::rule<ScannerType> ExpressionOrAssignment;
		boost::spirit::classic::rule<ScannerType> Statement;
		boost::spirit::classic::rule<ScannerType> PreOperatorStatement;
		boost::spirit::classic::rule<ScannerType> PostOperatorStatement;
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
		boost::spirit::classic::rule<ScannerType> PostfixEntity;

		boost::spirit::classic::rule<ScannerType> Program;

		boost::spirit::classic::stored_rule<ScannerType> OpAssignmentIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> InfixIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> UnaryPrefixIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> PreOperator;
		boost::spirit::classic::stored_rule<ScannerType> PostOperator;
		boost::spirit::classic::stored_rule<ScannerType> EntityIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> ChainedEntityIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> PostfixEntityOpenerIdentifier;
		boost::spirit::classic::stored_rule<ScannerType> PostfixEntityCloserIdentifier;

		boost::spirit::classic::guard<RecoverableException> GeneralExceptionGuard;
		boost::spirit::classic::guard<TypeMismatchException> TypeMismatchExceptionGuard;

		boost::spirit::classic::guard<MalformedStatementException> MalformedStatementExceptionGuard;
		boost::spirit::classic::assertion<MalformedStatementException> ExpectWellFormedStatement;

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
		// Register a unary prefix operator
		//
		void AddUnaryPrefix(const std::string& opname)
		{
			UnaryPrefixIdentifier = boost::spirit::classic::strlit<>(opname.c_str()) | UnaryPrefixIdentifier.copy();
		}

		//
		// Register a pre-operator
		//
		void AddPreOperator(const std::string& opname)
		{
			PreOperator = boost::spirit::classic::strlit<>(opname.c_str()) | PreOperator.copy();
		}

		//
		// Register a post-operator
		//
		void AddPostOperator(const std::string& opname)
		{
			PostOperator = boost::spirit::classic::strlit<>(opname.c_str()) | PostOperator.copy();
		}

		//
		// Register an inline entity
		//
		void AddInlineEntity(const std::string& entityname)
		{
			EntityIdentifier = boost::spirit::classic::strlit<>(entityname.c_str()) | EntityIdentifier.copy();
		}

		//
		// Register a chained entity
		//
		void AddChainedEntity(const std::string& entityname)
		{
			ChainedEntityIdentifier = boost::spirit::classic::strlit<>(entityname.c_str()) | ChainedEntityIdentifier.copy();
		}

		//
		// Register a postfix entity
		//
		void AddPostfixEntity(const std::string& entityname)
		{
			PostfixEntityOpenerIdentifier = boost::spirit::classic::strlit<>(entityname.c_str()) | PostfixEntityOpenerIdentifier.copy();
		}

		//
		// Register a postfix entity closer
		//
		void AddPostfixCloser(const std::string& entityname)
		{
			PostfixEntityCloserIdentifier = boost::spirit::classic::strlit<>(entityname.c_str()) | PostfixEntityCloserIdentifier.copy();
		}

		//
		// Register an op-assign operator
		//
		void AddOpAssignOperator(const std::string& identifier)
		{
			OpAssignmentIdentifier = boost::spirit::classic::strlit<>(identifier.c_str()) | OpAssignmentIdentifier.copy();
		}
	};

	SemanticActionInterface& Bindings;
	const IdentifierTable& Identifiers;

};

