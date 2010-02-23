//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Grammar for performing the first compile pass
//

#pragma once


// Dependencies
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_actor.hpp>
#include <boost/spirit/include/classic_stored_rule.hpp>

#include "Parser/Grammars/Base.h"
#include "Parser/Grammars/Util.h"

#include "Parser/Parse Functors/Aliases.h"
#include "Parser/Parse Functors/Blocks.h"
#include "Parser/Parse Functors/Debug.h"
#include "Parser/Parse Functors/Extensions.h"
#include "Parser/Parse Functors/Externals.h"
#include "Parser/Parse Functors/FlowControl.h"
#include "Parser/Parse Functors/Functions.h"
#include "Parser/Parse Functors/FunctionSignatures.h"
#include "Parser/Parse Functors/Globals.h"
#include "Parser/Parse Functors/Infix.h"
#include "Parser/Parse Functors/Structures.h"
#include "Parser/Parse Functors/Tuples.h"

#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Error Handling/SyntaxErrors.h"


namespace Parser
{

	//
	// Preprocess grammar for the Epoch language.
	// This grammar takes care of setting up lexical scopes, with
	// their contained variable and function definitions. It also
	// manages the bulk of syntax error detection and provides an
	// early-out path for programs that are not well-formed.
	//
	struct EpochGrammarPreProcess : public EpochGrammarBase, public boost::spirit::classic::grammar<EpochGrammarPreProcess>
	{
		//
		// Construct the grammar and bind it to a state tracker
		//
		EpochGrammarPreProcess(Parser::ParserState& state)
			: EpochGrammarBase(state)
		{ }

		//
		// Preprocess grammar definition
		//
		template <typename ScannerType>
		struct definition
		{
			definition(const EpochGrammarPreProcess& self)
				:
				  // Character tokens
				  COLON(':'), OPENPARENS('('), CLOSEPARENS(')'), OPENBRACE('{'), CLOSEBRACE('}'), COMMA(','), QUOTE('\"'), NEGATE('-'),

				  // String tokens: libraries and extensions
				  LIBRARY(KEYWORD(Library)),
				  EXTENSION(KEYWORD(Extension)),
				  
				  // String tokens: functions
				  FUNCTIONARROW(KEYWORD(FunctionArrow)),
				  NULLFUNCTIONARROW(KEYWORD(NullFunctionArrow)),
				  EXTERNAL(KEYWORD(External)),

				  // String tokens: flow control
				  IF(KEYWORD(If)), ELSEIF(KEYWORD(ElseIf)), ELSE(KEYWORD(Else)),
				  DO(KEYWORD(Do)), WHILE(KEYWORD(While)),

				  // String tokens: types
				  INTEGER(KEYWORD(Integer)), INTEGER16(KEYWORD(Integer16)), STRING(KEYWORD(String)), BOOLEAN(KEYWORD(Boolean)), REAL(KEYWORD(Real)),
				  TUPLE(KEYWORD(Tuple)), STRUCTURE(KEYWORD(Structure)), BUFFER(KEYWORD(Buffer)), ARRAY(KEYWORD(Array)),

				  // String tokens: parameter annotations
				  REFERENCE(KEYWORD(Reference)),
				  FUNCTION(KEYWORD(Function)),
				  CONSTANT(KEYWORD(Constant)),

				  // String tokens: built-in operations
				  ASSIGN(KEYWORD(Assign)),
				  CAST(KEYWORD(Cast)),
				  READTUPLE(KEYWORD(ReadTuple)),
				  WRITETUPLE(KEYWORD(AssignTuple)),
				  READSTRUCTURE(KEYWORD(ReadStructure)),
				  WRITESTRUCTURE(KEYWORD(AssignStructure)),
				  SIZEOF(KEYWORD(SizeOf)),
				  LENGTH(KEYWORD(Length)),
				  MEMBER(KEYWORD(Member)),
				  MESSAGE(KEYWORD(Message)),
				  MAP(KEYWORD(Map)),
				  REDUCE(KEYWORD(Reduce)),

				  // String tokens: special arithmetic operators
				  INCREMENT(OPERATOR(Increment)),
				  DECREMENT(OPERATOR(Decrement)),
				  ADDASSIGN(OPERATOR(AddAssign)),
				  SUBTRACTASSIGN(OPERATOR(SubtractAssign)),
				  MULTIPLYASSIGN(OPERATOR(MultiplyAssign)),
				  DIVIDEASSIGN(OPERATOR(DivideAssign)),
				  CONCATASSIGN(OPERATOR(ConcatAssign)),

				  // String tokens: multiprocessing
				  TASK(KEYWORD(Task)),
				  CALLER(KEYWORD(Caller)),
				  SENDER(KEYWORD(Sender)),
				  ACCEPTMESSAGE(KEYWORD(AcceptMessage)),
				  RESPONSEMAP(KEYWORD(ResponseMap)),
				  FUTURE(KEYWORD(Future)),
				  THREAD(KEYWORD(Thread)),
				  THREADPOOL(KEYWORD(ThreadPool)),

				  // String tokens: dynamic syntax
				  INFIXDECL(KEYWORD(Infix)),
				  ALIASDECL(KEYWORD(Alias)),

				  // String tokens: miscellaneous
				  TRUETOKEN(KEYWORD(True)), FALSETOKEN(KEYWORD(False)),
				  GLOBAL(KEYWORD(Global)),
				  HEXPREFIX(KEYWORD(HexPrefix)),
				  NOT(OPERATOR(Not)),
				  MEMBEROPERATOR(OPERATOR(Member)),

				  // String tokens: debugging
				  CRASHPARSER("@@crash_the_parser@@"),

				  // Syntax error descriptors
				  ExpectStatementOrControl(SYNTAXERROR_STATEMENTORCONTROL),
				  ExpectFunctionParameters(SYNTAXERROR_FUNCTIONDEFINITION),
				  ExpectBlock(SYNTAXERROR_BLOCK),
				  ExpectVariableDefinition(SYNTAXERROR_VARIABLEDEFINITION),
				  ExpectExpression(SYNTAXERROR_EXPRESSION),
				  ExpectQuote(SYNTAXERROR_QUOTE),
				  ExpectCloseBrace(SYNTAXERROR_CLOSEBRACE),
				  ExpectOpenParens(SYNTAXERROR_OPENPARENS),
				  ExpectCloseParens(SYNTAXERROR_CLOSEPARENS),
				  ExpectIntegerLiteral(SYNTAXERROR_INTEGERLITERAL),
				  ExpectRealLiteral(SYNTAXERROR_REALLITERAL),
				  ExpectBooleanLiteral(SYNTAXERROR_BOOLEANLITERAL),
				  ExpectStringLiteral(SYNTAXERROR_STRINGLITERAL),
				  ExpectComma(SYNTAXERROR_COMMA)
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

				HexLiteral
					= (HEXPREFIX) >> (+(hex_p))
					;

				RealLiteral
					= (!NEGATE) >> (+(digit_p)) >> '.' >> (+(digit_p))
					;

				StringLiteral
					= SyntaxErrorGuard
						(
						 QUOTE >>
							(lexeme_d[*(anychar_p - QUOTE - '\n')]) >>
						 ExpectQuote(QUOTE)
						)
					  [SyntaxErrorHandler(self.State)]
					;

				BooleanLiteral
					= TRUETOKEN
					| FALSETOKEN
					;

				LiteralValue
					= HexLiteral
					| RealLiteral
					| IntegerLiteral
					| StringLiteral
				    | BooleanLiteral
					;

				PassedParameterBase
					= !(NOT | NEGATE) >> Operation
					| !(NOT) >> LiteralValue
					| !(NOT | NEGATE) >> StringIdentifier
					;

				PassedParameterInfixList
					= PassedParameterBase >> (*(InfixOperator >> PassedParameterBase))
					;

				PassedParameter
					= PassedParameterInfixList
					;

				OperationParameter
					= PassedParameter % COMMA;
					;

				ReadStructureHelper
					= (READSTRUCTURE >> OPENPARENS >> StringIdentifier >> COMMA >> StringIdentifier >> CLOSEPARENS)
					| (READSTRUCTURE >> OPENPARENS >> PassedParameter >> COMMA >> StringIdentifier >> CLOSEPARENS)
					;

				WriteStructureHelper
					= (WRITESTRUCTURE >> OPENPARENS >> (StringIdentifier - MEMBER) >> COMMA >> StringIdentifier >> COMMA >> PassedParameter >> CLOSEPARENS)
					| (WRITESTRUCTURE >> OPENPARENS >> PassedParameter >> COMMA >> StringIdentifier >> COMMA >> PassedParameter >> CLOSEPARENS)
					;

				MemberHelper
					= (MEMBER >> OPENPARENS >> (StringIdentifier - MEMBER) >> COMMA >> StringIdentifier >> CLOSEPARENS)
					| (MEMBER >> OPENPARENS >> PassedParameter >> COMMA >> StringIdentifier >> CLOSEPARENS)
					;

				MessageHelper
					= MESSAGE >> OPENPARENS >>
						(
						    (CALLER >> OPENPARENS >> CLOSEPARENS)
						  | (SENDER >> OPENPARENS >> CLOSEPARENS)
						  |	PassedParameter
						) >>
						COMMA >> StringIdentifier >>
						OPENPARENS >>
							!OperationParameter >>
						CLOSEPARENS >> CLOSEPARENS
					;

				AcceptMessageHelper
					= (ACCEPTMESSAGE >> OPENPARENS >>
						(
							((StringIdentifier) >> CLOSEPARENS)
						  | (MessageDispatch >> CLOSEPARENS)
						)
					   )
					;

				MessageDispatch
					= StringIdentifier >>
						OPENPARENS >>
							*((
								(INTEGER >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (INTEGER16 >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (REAL >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (BOOLEAN >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (STRING >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							) % COMMA)
						>> CLOSEPARENS
						>> NULLFUNCTIONARROW
						>> CodeBlock
					;

				ResponseMapHelper
					= RESPONSEMAP >> OPENPARENS >> StringIdentifier >> CLOSEPARENS
					  >> OPENBRACE
						>> *(MessageDispatch)
					  >> CLOSEBRACE
					;

				IncrementDecrementHelper
					= (INCREMENT >> StringIdentifier)
					| (DECREMENT >> StringIdentifier)
					| (StringIdentifier >> INCREMENT)
					| (StringIdentifier >> DECREMENT)
					;

				OpAssignmentHelper
					= StringIdentifier
						>>
						(
							ADDASSIGN
						  | SUBTRACTASSIGN
						  | MULTIPLYASSIGN
						  | DIVIDEASSIGN
						  | CONCATASSIGN
						)
						>>
						PassedParameter
					;

				Operation
					= CRASHPARSER[CrashParser(self.State)]
					| (CAST >> OPENPARENS >> (TypeKeywords) >> COMMA >> PassedParameter >> CLOSEPARENS)
					| (ASSIGN >> OPENPARENS >> StringIdentifier >> COMMA >> PassedParameter >> CLOSEPARENS)
					| (READTUPLE >> OPENPARENS >> StringIdentifier >> COMMA >> StringIdentifier >> CLOSEPARENS)
					| (WRITETUPLE >> OPENPARENS >> StringIdentifier >> COMMA >> StringIdentifier >> COMMA >> PassedParameter >> CLOSEPARENS)
					| ReadStructureHelper
					| WriteStructureHelper
					| (SIZEOF >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
					| (LENGTH >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
					| (FUTURE >> OPENPARENS >> StringIdentifier >> COMMA >> PassedParameter >> CLOSEPARENS)
					| ((MAP | REDUCE) >> OPENPARENS >> PassedParameter >> COMMA >> StringIdentifier >> CLOSEPARENS)
					| MemberHelper
					| MessageHelper
					| AcceptMessageHelper
					| IncrementDecrementHelper
					| OpAssignmentHelper
					| (((StringIdentifier - ControlKeywords) - OtherKeywords) >> OPENPARENS >> *OperationParameter >> CLOSEPARENS)
					| (OPENPARENS >> +OperationParameter >> CLOSEPARENS)
					;

				Task
					= (TASK >> OPENPARENS >> PassedParameter >> CLOSEPARENS >> CodeBlock)
					;

				ThreadBlock
					= (THREAD >> OPENPARENS >> PassedParameter >> COMMA >> PassedParameter >> CLOSEPARENS >> CodeBlock)
					;

				ThreadPool
					= (THREADPOOL >> OPENPARENS >> PassedParameter >> COMMA >> PassedParameter >> CLOSEPARENS)
					;

				InfixAssignmentHelper
					= ((StringIdentifier >> +(MEMBEROPERATOR >> StringIdentifier)) >> boost::spirit::classic::strlit<>(OPERATOR(Assign)) >> PassedParameter)
					| (*((StringIdentifier >> boost::spirit::classic::strlit<>(OPERATOR(Assign)))) >> (PassedParameter))
					;

				LanguageExtensionBlock
					= LanguageExtensionKeywords >> CodeBlock
					;

				CodeBlockContents
					= LanguageExtensionBlock
					| Control
					| TupleDefinition
					| StructureDefinition
					| VariableDefinition
					| Task
					| ResponseMapHelper
					| Operation
					| InfixAssignmentHelper
					| CodeBlock
					;

				CodeBlock
					= (
						OPENBRACE[EnterBlockPP(self.State)] >>
							(*CodeBlockContents) >>
						CLOSEBRACE[ExitBlockPP(self.State)]
					  )
					;

				ControlSimple
					= IF[RegisterControl(self.State, true)] >> ExpectExpression(OPENPARENS >> PassedParameter >> CLOSEPARENS) >> CodeBlock >> *(ELSEIF[RegisterControl(self.State, true)] >> ExpectExpression(OPENPARENS >> PassedParameter >> CLOSEPARENS) >> CodeBlock) >> !(ELSE[RegisterControl(self.State, true)] >> CodeBlock)
					| WHILE[RegisterControl(self.State, true)] >> ExpectExpression(OPENPARENS >> PassedParameter >> CLOSEPARENS) >> CodeBlock
					;

				ControlWithEnding
					= DO[RegisterControl(self.State, true)] >> CodeBlock >> WHILE >> ExpectExpression(OPENPARENS >> PassedParameter >> CLOSEPARENS[PopDoWhileLoopPP(self.State)])
					;

				ControlKeywords
					= IF
					| DO
					| WHILE
					| ELSE
					| ELSEIF
					;

				TypeKeywords
					= INTEGER
					| INTEGER16
					| STRING
					| BOOLEAN
					| REAL
					| BUFFER
					| ARRAY
					;

				OtherKeywords
					= ASSIGN
					| CAST
				    | READTUPLE
					| WRITETUPLE
					| READSTRUCTURE
					| WRITESTRUCTURE
					| SIZEOF
					| LENGTH
					| MEMBER
					| TASK
					| MESSAGE
					| ACCEPTMESSAGE
					| RESPONSEMAP
					| FUTURE
					| MAP
					| REDUCE
					| THREAD
					| THREADPOOL
					| LanguageExtensionKeywords
					;

				Control
					= ControlWithEnding
					| ControlSimple
					;

				VariableDefinition
					= !CONSTANT >> INTEGER >> (OPENPARENS >> StringIdentifier >> ExpectComma(COMMA) >> PassedParameter >> CLOSEPARENS)
					| !CONSTANT >> INTEGER16 >> (OPENPARENS >> StringIdentifier >> ExpectComma(COMMA) >> PassedParameter >> CLOSEPARENS)
					| !CONSTANT >> STRING >> (OPENPARENS >> StringIdentifier >> ExpectComma(COMMA) >> PassedParameter >> CLOSEPARENS)
					| !CONSTANT >> BOOLEAN >> (OPENPARENS >> StringIdentifier >> ExpectComma(COMMA) >> PassedParameter >> CLOSEPARENS)
					| !CONSTANT >> REAL >> (OPENPARENS >> StringIdentifier >> ExpectComma(COMMA) >> PassedParameter >> CLOSEPARENS)
					| !CONSTANT >> ARRAY >> (OPENPARENS >> StringIdentifier >> ExpectComma(COMMA) >> (TypeKeywords | OperationParameter) >> CLOSEPARENS)
					;

				TupleDefinition
					= TUPLE >> StringIdentifier[RegisterTupleType(self.State)] >> COLON >> OPENPARENS
						>>  (
								(INTEGER >> OPENPARENS >> StringIdentifier[RegisterTupleIntegerMember(self.State)] >> CLOSEPARENS)
							  | (INTEGER16 >> OPENPARENS >> StringIdentifier[RegisterTupleInt16Member(self.State)] >> CLOSEPARENS)
							  | (REAL >> OPENPARENS >> StringIdentifier[RegisterTupleRealMember(self.State)] >> CLOSEPARENS)
							  | (BOOLEAN >> OPENPARENS >> StringIdentifier[RegisterTupleBooleanMember(self.State)] >> CLOSEPARENS)
							  | (STRING >> OPENPARENS >> StringIdentifier[RegisterTupleStringMember(self.State)] >> CLOSEPARENS)
							) % COMMA
					  >> CLOSEPARENS[FinishTupleType(self.State)]
					;

				StructureDefinition
					= STRUCTURE >> StringIdentifier[RegisterStructureType(self.State)] >> COLON >> OPENPARENS
						>>  (
								(INTEGER >> OPENPARENS >> StringIdentifier[RegisterStructureIntegerMember(self.State)] >> CLOSEPARENS)
							  |	(INTEGER16 >> OPENPARENS >> StringIdentifier[RegisterStructureInt16Member(self.State)] >> CLOSEPARENS)
							  | (REAL >> OPENPARENS >> StringIdentifier[RegisterStructureRealMember(self.State)] >> CLOSEPARENS)
							  | (BOOLEAN >> OPENPARENS >> StringIdentifier[RegisterStructureBooleanMember(self.State)] >> CLOSEPARENS)
							  | (STRING >> OPENPARENS >> StringIdentifier[RegisterStructureStringMember(self.State)] >> CLOSEPARENS)
							  | ((StringIdentifier - TypeKeywords)[RegisterStructureUnknownTypeName(self.State)] >> OPENPARENS >> StringIdentifier[RegisterStructureUnknown(self.State)] >> CLOSEPARENS)
							) % COMMA
					  >> CLOSEPARENS[FinishStructureType(self.State)]
					;

				HigherOrderFunctionHelper
					= (
						FUNCTION >>
							StringIdentifier[RegisterFunctionSignatureName(self.State)] >> COLON >>
								((OPENPARENS
										>> (((INTEGER[RegisterFunctionSignatureIntegerParam(self.State)]
										   | INTEGER16[RegisterFunctionSignatureInteger16Param(self.State)]
										   | STRING[RegisterFunctionSignatureStringParam(self.State)]
										   | BOOLEAN[RegisterFunctionSignatureBooleanParam(self.State)]
										   | REAL[RegisterFunctionSignatureRealParam(self.State)]
										   | StringIdentifier[RegisterFunctionSignatureUnknownParam(self.State)]) >> !(REFERENCE[RegisterFunctionSignatureParamIsReference(self.State)])) % COMMA)
										>> CLOSEPARENS)
								 | (OPENPARENS >> CLOSEPARENS))
								>> FUNCTIONARROW >>
								((OPENPARENS
										>> (INTEGER[RegisterFunctionSignatureIntegerReturn(self.State)]
										   | INTEGER16[RegisterFunctionSignatureInteger16Return(self.State)]
										   | STRING[RegisterFunctionSignatureStringReturn(self.State)]
										   | BOOLEAN[RegisterFunctionSignatureBooleanReturn(self.State)]
										   | REAL[RegisterFunctionSignatureRealReturn(self.State)]
										   | StringIdentifier[RegisterFunctionSignatureUnknownReturn(self.State)]) % COMMA
										>> CLOSEPARENS[RegisterFunctionSignatureEnd(self.State)])
								 | (OPENPARENS >> CLOSEPARENS[RegisterFunctionSignatureEnd(self.State)]))
					  )
					;

				FunctionReturns
					= (  (INTEGER >> OPENPARENS >> StringIdentifier[RegisterIntegerReturn(self.State)] >> COMMA >> IntegerLiteral[RegisterIntegerReturnValue(self.State)] >> CLOSEPARENS)
				 	   | (INTEGER16 >> OPENPARENS >> StringIdentifier[RegisterInt16Return(self.State)] >> COMMA >> IntegerLiteral[RegisterInt16ReturnValue(self.State)] >> CLOSEPARENS)
					   | (STRING >> OPENPARENS >> StringIdentifier[RegisterStringReturn(self.State)] >> COMMA >> StringLiteral[RegisterStringReturnValue(self.State)] >> CLOSEPARENS)
					   | (BOOLEAN >> OPENPARENS >> StringIdentifier[RegisterBooleanReturn(self.State)] >> COMMA >> BooleanLiteral[RegisterBooleanReturnValue(self.State)] >> CLOSEPARENS)
					   | (REAL >> OPENPARENS >> StringIdentifier[RegisterRealReturn(self.State)] >> COMMA >> RealLiteral[RegisterRealReturnValue(self.State)] >> CLOSEPARENS)
					   | ((StringIdentifier - FUNCTION)[RegisterUnknownReturn(self.State)] >> OPENPARENS >> StringIdentifier[RegisterUnknownReturnName(self.State)] >> COMMA >> OperationParameter[ExitUnknownReturnConstructor(self.State)] >> CLOSEPARENS)
					  ) % COMMA
					;

				FunctionDefinition
					= SyntaxErrorGuard
					(
						(!(INFIXDECL[RegisterUserDefinedInfix(self.State)])) >>
						(StringIdentifier - EXTERNAL - TUPLE - STRUCTURE - FUNCTION - GLOBAL - INFIXDECL)[RegisterFunctionPP(self.State)] >>
							ExpectFunctionParameters
							(
								COLON >>
								((OPENPARENS >>
										(((INTEGER[RegisterIntegerParam(self.State)]
										 | INTEGER16[RegisterInt16Param(self.State)]
										 | STRING[RegisterStringParam(self.State)]
										 | BOOLEAN[RegisterBooleanParam(self.State)]
										 | REAL[RegisterRealParam(self.State)]
										 | (StringIdentifier - FUNCTION)[RegisterUnknownParam(self.State)])
									>> !(REFERENCE[RegisterParamIsReference(self.State)])
									>> OPENPARENS >> StringIdentifier[RegisterParamName(self.State)] >> CLOSEPARENS)
								 | HigherOrderFunctionHelper)
								 % COMMA >>
								CLOSEPARENS)
								| (OPENPARENS >> CLOSEPARENS))
								  >> FUNCTIONARROW
								  >> (
										(
											OPENPARENS[RegisterBeginningOfFunctionReturns(self.State)] >> FunctionReturns >> CLOSEPARENS
										)
									  |
										(OPENPARENS >> CLOSEPARENS[RegisterNullReturn(self.State)])
									)
							) >> CodeBlock
					)[SyntaxErrorHandler(self.State)]
					;

				ExternalDeclaration
					= EXTERNAL >> StringLiteral[RegisterExternalFunctionDLL(self.State)] >> StringIdentifier[RegisterExternalFunctionName(self.State)] >> COLON
					  >> OPENPARENS >> 
							(!(
								  (
									( INTEGER[RegisterExternalIntegerParam(self.State)]
									| INTEGER16[RegisterExternalInt16Param(self.State)]
									| STRING[RegisterExternalStringParam(self.State)]
									| BOOLEAN[RegisterExternalBooleanParam(self.State)]
									| REAL[RegisterExternalRealParam(self.State)]
									| BUFFER[RegisterExternalBufferParam(self.State)]
									)
									>> !(REFERENCE[RegisterParamIsReference(self.State)])
									>> OPENPARENS >> StringIdentifier[RegisterExternalParamName(self.State)] >> CLOSEPARENS
								  ) 
							    | (
									HigherOrderFunctionHelper
								  )
								| (
									StringIdentifier[RegisterExternalParamAddressType(self.State)] >> (!REFERENCE[RegisterParamIsReference(self.State)]) >> OPENPARENS >>
									StringIdentifier[RegisterExternalParamAddressName(self.State)] >> CLOSEPARENS
								  )
							)
							% COMMA)
							>> CLOSEPARENS
					  >> FUNCTIONARROW
					  >> ((OPENPARENS >> 
										(  INTEGER[RegisterExternalIntegerReturn(self.State)]
										 | INTEGER16[RegisterExternalInt16Return(self.State)]
										 | STRING[RegisterExternalStringReturn(self.State)]
										 | BOOLEAN[RegisterExternalBooleanReturn(self.State)]
										 | REAL[RegisterExternalRealReturn(self.State)]
										 | BUFFER[RegisterExternalBufferReturn(self.State)])
									  >> CLOSEPARENS)
						| (OPENPARENS >> CLOSEPARENS[RegisterExternalNullReturn(self.State)]))
					;

				LibraryImport
					= LIBRARY >> OPENPARENS >> StringLiteral[RegisterLibrary(self.State)] >> CLOSEPARENS
					;

				ExtensionImport
					= EXTENSION >> OPENPARENS >> StringLiteral[RegisterExtension<definition<ScannerType> >(self.State, *this)] >> CLOSEPARENS
					;

				GlobalBlock
					= GLOBAL >> OPENBRACE >>
					  (
						*(VariableDefinition | TupleDefinition | StructureDefinition | Operation)
					  )
					  >> CLOSEBRACE
					;

				Program
					= *(ExternalDeclaration | GlobalBlock | LibraryImport | ExtensionImport | HigherOrderFunctionHelper | TupleDefinition | StructureDefinition | AliasDeclaration | FunctionDefinition)
					;

				InfixOperator = boost::spirit::classic::strlit<>(OPERATOR(Add))
							  | boost::spirit::classic::strlit<>(OPERATOR(Subtract))
							  | boost::spirit::classic::strlit<>(OPERATOR(Multiply))
							  | boost::spirit::classic::strlit<>(OPERATOR(Divide))
							  | boost::spirit::classic::strlit<>(OPERATOR(GreaterEqual))
							  | boost::spirit::classic::strlit<>(OPERATOR(Greater))
							  | boost::spirit::classic::strlit<>(OPERATOR(LessEqual))
							  | boost::spirit::classic::strlit<>(OPERATOR(Less))
							  | boost::spirit::classic::strlit<>(OPERATOR(Equal))
							  | boost::spirit::classic::strlit<>(OPERATOR(NotEqual))
							  | boost::spirit::classic::strlit<>(OPERATOR(Concat))
							  | boost::spirit::classic::strlit<>(OPERATOR(And))
							  | boost::spirit::classic::strlit<>(OPERATOR(Or))
							  | boost::spirit::classic::strlit<>(OPERATOR(Xor))
							  | boost::spirit::classic::strlit<>(OPERATOR(Member))
							  ;

				AliasDeclaration
					= ALIASDECL >> OPENPARENS >> StringIdentifier[RegisterAliasName(self.State)] >> COMMA >> StringIdentifier[RegisterAliasType(self.State)] >> CLOSEPARENS
					;
			}

			// Character tokens
			boost::spirit::classic::chlit<> COLON, OPENPARENS, CLOSEPARENS, OPENBRACE, CLOSEBRACE, COMMA, QUOTE, NEGATE;

			// String tokens
			boost::spirit::classic::strlit<> FUNCTIONARROW, IF, WHILE, ELSE, DO, INTEGER, STRING, BOOLEAN, REAL, TRUETOKEN, FALSETOKEN, EXTERNAL, NULLFUNCTIONARROW;
			boost::spirit::classic::strlit<> TUPLE, STRUCTURE, INTEGER16, REFERENCE, FUNCTION, LIBRARY, GLOBAL, ELSEIF, CONSTANT, HEXPREFIX, TASK, ACCEPTMESSAGE;
			boost::spirit::classic::strlit<> RESPONSEMAP, INFIXDECL, CRASHPARSER, NOT, BUFFER, ASSIGN, CAST, READTUPLE, WRITETUPLE, READSTRUCTURE, WRITESTRUCTURE;
			boost::spirit::classic::strlit<> SIZEOF, LENGTH, MEMBER, MESSAGE, FUTURE, MAP, REDUCE, CALLER, SENDER, ALIASDECL, INCREMENT, DECREMENT, THREADPOOL;
			boost::spirit::classic::strlit<> ADDASSIGN, SUBTRACTASSIGN, MULTIPLYASSIGN, DIVIDEASSIGN, CONCATASSIGN, ARRAY, MEMBEROPERATOR, EXTENSION, THREAD;

			// Parser rules
			boost::spirit::classic::rule<ScannerType> StringIdentifier, FunctionDefinition, PassedParameter, OperationParameter, Operation, CodeBlock, Program;
			boost::spirit::classic::rule<ScannerType> LiteralValue, IntegerLiteral, StringLiteral, Control, ControlSimple, ControlWithEnding, LibraryImport, AliasDeclaration;
			boost::spirit::classic::rule<ScannerType> ControlKeywords, TypeKeywords, VariableDefinition, BooleanLiteral, CodeBlockContents, GlobalBlock, PassedParameterInfixList;
			boost::spirit::classic::rule<ScannerType> ExternalDeclaration, RealLiteral, TupleDefinition, StructureDefinition, HigherOrderFunctionHelper, MessageDispatch;
			boost::spirit::classic::rule<ScannerType> HexLiteral, Task, AcceptMessageHelper, ResponseMapHelper, PassedParameterBase, InfixOperator, ThreadPool, ThreadBlock;
			boost::spirit::classic::rule<ScannerType> InfixAssignmentHelper, OtherKeywords, ReadStructureHelper, WriteStructureHelper, MemberHelper, MessageHelper;
			boost::spirit::classic::rule<ScannerType> IncrementDecrementHelper, OpAssignmentHelper, LanguageExtensionBlock, ExtensionImport, FunctionReturns;

			boost::spirit::classic::stored_rule<ScannerType> LanguageExtensionKeywords;

			// Syntax error traps
			boost::spirit::classic::assertion<SyntaxErrors> ExpectStatementOrControl, ExpectFunctionParameters, ExpectBlock, ExpectVariableDefinition;
			boost::spirit::classic::assertion<SyntaxErrors> ExpectExpression, ExpectQuote, ExpectCloseBrace, ExpectOpenParens, ExpectCloseParens;
			boost::spirit::classic::assertion<SyntaxErrors> ExpectIntegerLiteral, ExpectRealLiteral, ExpectBooleanLiteral, ExpectStringLiteral;
			boost::spirit::classic::assertion<SyntaxErrors> ExpectComma;
			boost::spirit::classic::guard<SyntaxErrors> SyntaxErrorGuard;

			//
			// Retrieve the parser's starting (root) symbol
			//
			const boost::spirit::classic::rule<ScannerType>& start() const
			{ return Program; }


			//
			// Helper for setting up user-defined language extensions
			//
			void AddLanguageExtension(const std::wstring& blockkeyword)
			{
				LanguageExtensionKeywords = LanguageExtensionKeywords.copy() | boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(blockkeyword.c_str()));
			}
		};
	};


}

#undef KEYWORD
#undef OPERATOR