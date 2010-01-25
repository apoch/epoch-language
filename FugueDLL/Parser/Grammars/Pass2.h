//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Grammar for performing the second compiler pass
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
#include "Parser/Parse Functors/Concurrency.h"
#include "Parser/Parse Functors/Debug.h"
#include "Parser/Parse Functors/Extensions.h"
#include "Parser/Parse Functors/Externals.h"
#include "Parser/Parse Functors/FlowControl.h"
#include "Parser/Parse Functors/Functions.h"
#include "Parser/Parse Functors/FunctionSignatures.h"
#include "Parser/Parse Functors/Globals.h"
#include "Parser/Parse Functors/Infix.h"
#include "Parser/Parse Functors/Operations.h"
#include "Parser/Parse Functors/Parameters.h"
#include "Parser/Parse Functors/Structures.h"
#include "Parser/Parse Functors/Tuples.h"
#include "Parser/Parse Functors/Utility.h"
#include "Parser/Parse Functors/Variables.h"

#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Error Handling/SyntaxErrors.h"


namespace Parser
{


	//
	// Primary grammar for the Epoch language.
	//
	// This grammar is responsible for actually converting the raw
	// source code into a series of wrapper objects that represent
	// the compiled code tree. This is accomplished via the use of
	// boost::spirit semantic action functors, which then call the
	// ParserState interface to build the actual code tree.
	//
	struct EpochGrammar : public EpochGrammarBase, public boost::spirit::classic::grammar<EpochGrammar>
	{
		//
		// Construct the grammar and bind it to a state tracker
		//
		EpochGrammar(Parser::ParserState& state)
			: EpochGrammarBase(state)
		{ }

		//
		// Main parse phase grammar definition
		//
		template <typename ScannerType>
		struct definition
		{
			definition(const EpochGrammar& self)
				:
				  // Character tokens
				  COLON(':'), OPENPARENS('('), CLOSEPARENS(')'), OPENBRACE('{'), CLOSEBRACE('}'), COMMA(','), QUOTE('\"'), NEGATE('-'),
				  
				  // String tokens
				  FUNCTIONARROW(KEYWORD(FunctionArrow)), NULLFUNCTIONARROW(KEYWORD(NullFunctionArrow)),
				  IF(KEYWORD(If)), WHILE(KEYWORD(While)), ELSE(KEYWORD(Else)),
				  DO(KEYWORD(Do)), INTEGER(KEYWORD(Integer)), STRING(KEYWORD(String)),
				  BOOLEAN(KEYWORD(Boolean)), REAL(KEYWORD(Real)),
				  TRUETOKEN(KEYWORD(True)), FALSETOKEN(KEYWORD(False)), EXTERNAL(KEYWORD(External)),
				  CAST(KEYWORD(Cast)), ASSIGN(KEYWORD(Assign)), TUPLE(KEYWORD(Tuple)), READTUPLE(KEYWORD(ReadTuple)),
				  WRITETUPLE(KEYWORD(AssignTuple)), STRUCTURE(KEYWORD(Structure)), READSTRUCTURE(KEYWORD(ReadStructure)),
				  WRITESTRUCTURE(KEYWORD(AssignStructure)), SIZEOF(KEYWORD(SizeOf)), INTEGER16(KEYWORD(Integer16)),
				  REFERENCE(KEYWORD(Reference)), FUNCTION(KEYWORD(Function)), LENGTH(KEYWORD(Length)), LIBRARY(KEYWORD(Library)),
				  GLOBAL(KEYWORD(Global)), ELSEIF(KEYWORD(ElseIf)), MEMBER(KEYWORD(Member)), CONSTANT(KEYWORD(Constant)),
				  HEXPREFIX(KEYWORD(HexPrefix)), TASK(KEYWORD(Task)), MESSAGE(KEYWORD(Message)), ACCEPTMESSAGE(KEYWORD(AcceptMessage)),
				  SENDER(KEYWORD(Sender)), CALLER(KEYWORD(Caller)), RESPONSEMAP(KEYWORD(ResponseMap)), INFIXDECL(KEYWORD(Infix)),
				  FUTURE(KEYWORD(Future)), REDUCE(KEYWORD(Reduce)), LIST(KEYWORD(List)), MAP(KEYWORD(Map)), VAR(KEYWORD(Var)),
				  NOT(OPERATOR(Not)), BUFFER(KEYWORD(Buffer)), ALIASDECL(KEYWORD(Alias)), MEMBEROPERATOR(OPERATOR(Member)),
				  EXTENSION(KEYWORD(Extension)), THREAD(KEYWORD(Thread)), THREADPOOL(KEYWORD(ThreadPool)),

				  // String tokens: special arithmetic operators
				  INCREMENT(OPERATOR(Increment)),
				  DECREMENT(OPERATOR(Decrement)),
				  ADDASSIGN(OPERATOR(AddAssign)),
				  SUBTRACTASSIGN(OPERATOR(SubtractAssign)),
				  MULTIPLYASSIGN(OPERATOR(MultiplyAssign)),
				  DIVIDEASSIGN(OPERATOR(DivideAssign)),
				  CONCATASSIGN(OPERATOR(ConcatAssign)),

				  CRASHPARSER("@@crash_the_parser@@")
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
					= QUOTE >> *(anychar_p - QUOTE) >> QUOTE
					;

				BooleanLiteral
					= TRUETOKEN
					| FALSETOKEN
					;

				LiteralValue
					= HexLiteral[PushHexLiteral(self.State)]
					| RealLiteral[PushRealLiteral(self.State)]
					| IntegerLiteral[PushIntegerLiteral(self.State)]
					| StringLiteral[PushStringLiteral(self.State)]
				    | BooleanLiteral[PushBooleanLiteral(self.State)]
					;

				PassedParameterBase
					= !(NOT[RegisterNotOperation(self.State)] | NEGATE[RegisterNegateOperation(self.State)]) >> Operation[RegisterOperationAsParameter(self.State)]
					| !(NOT[RegisterNotOperation(self.State)]) >> LiteralValue[UndoNegateOperation(self.State)]
					| !(NOT[RegisterNotOperation(self.State)] | NEGATE[RegisterNegateOperation(self.State)]) >> StringIdentifier[PushIdentifierAsParameter(self.State)]
					;

				InfixHelper
					= (MEMBEROPERATOR >> StringIdentifier[RegisterMemberAccess(self.State)])
					| (InfixOperator[ResetMemberAccess(self.State)][RegisterInfixOperator(self.State)][ResetMemberAccessRValue(self.State)] >> PassedParameterBase[RegisterInfixOperand(self.State)])
					;

				PassedParameterInfixList
					= PassedParameterBase[RegisterInfixOperand(self.State)] >> (*InfixHelper)
					;

				PassedParameter
					= PassedParameterInfixList[TerminateInfixExpression(self.State)]
					;

				OperationParameter
					= PassedParameter % COMMA[ResetInfixTracking(self.State)]
					;

				ReadStructureHelper
					= (READSTRUCTURE >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					| (READSTRUCTURE >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					;

				WriteStructureHelper
					= (WRITESTRUCTURE >> OPENPARENS[StartCountingParams(self.State)] >> (StringIdentifier - MEMBER)[PushIdentifierNoStack(self.State)] >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> PassedParameter >> CLOSEPARENS)
					| (WRITESTRUCTURE >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> PassedParameter >> CLOSEPARENS)[ResetMemberLevel(self.State)]
					;

				MemberHelper
					= (MEMBER >> OPENPARENS[StartCountingParams(self.State)] >> (StringIdentifier - MEMBER)[PushIdentifierNoStack(self.State)] >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					| (MEMBER >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					;

				MessageHelper
					= MESSAGE >> OPENPARENS[StartCountingParams(self.State)] >>
						(
						    (CALLER >> OPENPARENS >> CLOSEPARENS)[PushCallerOperation(self.State)]
						  | (SENDER >> OPENPARENS >> CLOSEPARENS)[PushSenderOperation(self.State)]
						  |	(PassedParameter)[CacheTailOperations(self.State)]
						) >>
						COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >>
						OPENPARENS[StartCountingParams(self.State)] >>
							!OperationParameter >>
						CLOSEPARENS >> CLOSEPARENS[PushCachedOperations(self.State)]
					;

				AcceptMessageHelper
					= (ACCEPTMESSAGE >> OPENPARENS[StartCountingParams(self.State)] >>
						(
							((StringIdentifier[SaveStringIdentifier(self.State, SavedStringSlot_AcceptMsg)]) >> CLOSEPARENS)[PushSavedIdentifier(self.State, SavedStringSlot_AcceptMsg)]
						  | (MessageDispatch >> CLOSEPARENS)
						)
					   )
					;

				MessageDispatch
					= StringIdentifier[PushIdentifierNoStack(self.State)][StartMessageParamScope(self.State)] >>
						OPENPARENS[StartCountingParams(self.State)] >>
							*((
								(INTEGER >> OPENPARENS >> StringIdentifier[RegisterIntegerMessageParam(self.State)] >> CLOSEPARENS)
							  | (INTEGER16 >> OPENPARENS >> StringIdentifier[RegisterInt16MessageParam(self.State)] >> CLOSEPARENS)
							  | (REAL >> OPENPARENS >> StringIdentifier[RegisterRealMessageParam(self.State)] >> CLOSEPARENS)
							  | (BOOLEAN >> OPENPARENS >> StringIdentifier[RegisterBooleanMessageParam(self.State)] >> CLOSEPARENS)
							  | (STRING >> OPENPARENS >> StringIdentifier[RegisterStringMessageParam(self.State)] >> CLOSEPARENS)
							) % COMMA)
						>> CLOSEPARENS
						>> NULLFUNCTIONARROW[RegisterUpcomingMessageDispatch(self.State)]
						>> CodeBlock
					;

				ResponseMapHelper
					= RESPONSEMAP >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS
					  >> OPENBRACE[BeginResponseMap(self.State)]
						>> *(MessageDispatch[RegisterResponseMapEntry(self.State)])
					  >> CLOSEBRACE[EndResponseMap(self.State)]
					;

				IncrementDecrementHelper
					= (INCREMENT >> StringIdentifier[SaveStringIdentifier(self.State, SavedStringSlot_IncDec)])[PreincrementVariable(self.State)]
					| (DECREMENT >> StringIdentifier[SaveStringIdentifier(self.State, SavedStringSlot_IncDec)])[PredecrementVariable(self.State)]
					| (StringIdentifier[SaveStringIdentifier(self.State, SavedStringSlot_IncDec)] >> INCREMENT)[PostincrementVariable(self.State)]
					| (StringIdentifier[SaveStringIdentifier(self.State, SavedStringSlot_IncDec)] >> DECREMENT)[PostdecrementVariable(self.State)]
					;

				OpAssignmentHelper
					= StringIdentifier[SaveStringIdentifier(self.State, SavedStringSlot_OpAssign)]
						>>
							(
								ADDASSIGN
							  | SUBTRACTASSIGN
							  | MULTIPLYASSIGN
							  | DIVIDEASSIGN
							  | CONCATASSIGN
							)[RegisterOpAssignmentOperator(self.State)]
						>>
						PassedParameter[PushSavedIdentifier(self.State, SavedStringSlot_OpAssign)][RegisterOpAssignment(self.State)]
					;
					
				Operation
					= CRASHPARSER[CrashParser(self.State)]
					| (CAST >> OPENPARENS[StartCountingParams(self.State)] >> (TypeKeywords[PushRawStringNoStack(self.State)] | UserDefinedTypeAliases[ResolveAliasAndPushNoStack(self.State)]) >> COMMA >> PassedParameter >> CLOSEPARENS)
					| (ASSIGN >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> PassedParameter >> CLOSEPARENS)
					| (READTUPLE >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					| (WRITETUPLE >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> PassedParameter >> CLOSEPARENS)
					| ReadStructureHelper
					| WriteStructureHelper
					| (SIZEOF >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					| (LENGTH >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					| (FUTURE >> OPENPARENS[StartCountingParams(self.State)] >> StringIdentifier[PushIdentifierNoStack(self.State)] >> COMMA >> PassedParameter >> !(COMMA[ResetInfixTracking(self.State)] >> PassedParameter) >> CLOSEPARENS)
					| ((MAP | REDUCE) >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> COMMA >> StringIdentifier[PushIdentifierNoStack(self.State)] >> CLOSEPARENS)
					| MemberHelper[IncrementMemberLevel(self.State)]
					| MessageHelper
					| AcceptMessageHelper
					| IncrementDecrementHelper
					| (((StringIdentifier - ControlKeywords) - OtherKeywords) >> OPENPARENS[StartCountingParams(self.State)] >> *OperationParameter >> CLOSEPARENS)
					| (OPENPARENS[StartCountingParams(self.State)] >> +OperationParameter >> CLOSEPARENS)[TerminateParenthetical(self.State)]
					;

				Task
					= (TASK >> (OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> CLOSEPARENS[BeginTaskCode(self.State)])[SaveTaskName(self.State)] >> CodeBlock)
					;

				Thread
					= (THREAD >> (OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> COMMA >> PassedParameter >> CLOSEPARENS[BeginThreadCode(self.State)])[SaveTaskName(self.State)] >> CodeBlock)
					;

				ThreadPool
					= (THREADPOOL >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> COMMA >> PassedParameter >> CLOSEPARENS)[RegisterThreadPool(self.State)]
					;

				InfixAssignmentHelper
					= ((StringIdentifier[SaveStringIdentifier(self.State, SavedStringSlot_InfixLValue)] >> +(MEMBEROPERATOR >> StringIdentifier[RegisterMemberLValueAccess(self.State)]) >> boost::spirit::classic::strlit<>(OPERATOR(Assign))[RegisterCompositeLValue(self.State)][ResetMemberAccessLValue(self.State)][ResetMemberAccessRValue(self.State)][StartCountingParams(self.State)] >> PassedParameter)[FinalizeCompositeAssignment(self.State)])
					| (+((StringIdentifier >> boost::spirit::classic::strlit<>(OPERATOR(Assign)))[RegisterInfixOperandAsLValue(self.State)]) >> PassedParameter)
					;

				LanguageExtensionBlock
					= LanguageExtensionKeywords[PushExtensionBlockKeyword(self.State)] >> CodeBlock
					;

				CodeBlockContents
					= LanguageExtensionBlock[RegisterExtensionBlock(self.State)]
					| Control
					| TupleDefinition
					| StructureDefinition
					| VariableDefinition
					| Task
					| ThreadPool
					| Thread
					| ResponseMapHelper
					| Operation[RegisterOperation(self.State)]
					| OpAssignmentHelper
					| InfixAssignmentHelper[PopParameterCount(self.State)]
					| CodeBlock
					;

				CodeBlock
					= OPENBRACE[EnterBlock(self.State)] >> *(CodeBlockContents) >> CLOSEBRACE[ExitBlock(self.State)]
					;

				ControlSimple
					= IF[RegisterControl(self.State, false)] >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> CLOSEPARENS >> CodeBlock >> *(ELSEIF[RegisterControl(self.State, false)] >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> CLOSEPARENS >> CodeBlock) >> !(ELSE[RegisterControl(self.State, false)] >> CodeBlock)
					| WHILE[RegisterControl(self.State, false)] >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> CLOSEPARENS[RegisterEndOfWhileLoopConditional(self.State)] >> CodeBlock
					;

				ControlWithEnding
					= DO[RegisterControl(self.State, false)] >> CodeBlock >> WHILE >> OPENPARENS[StartCountingParams(self.State)] >> PassedParameter >> CLOSEPARENS[PopDoWhileLoop(self.State)]
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
					| LIST
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
					= !CONSTANT[RegisterUpcomingConstant(self.State)] >> VAR >> OPENPARENS[RegisterUpcomingInferredVariable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> PassedParameter[RegisterVariableValue(self.State)] >> CLOSEPARENS

					| !CONSTANT[RegisterUpcomingConstant(self.State)] >> INTEGER >> OPENPARENS[RegisterUpcomingIntegerVariable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> PassedParameter[RegisterVariableValue(self.State)] >> CLOSEPARENS

					| !CONSTANT[RegisterUpcomingConstant(self.State)] >> INTEGER16 >> OPENPARENS[RegisterUpcomingInt16Variable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> PassedParameter[RegisterVariableValue(self.State)] >> CLOSEPARENS

					| !CONSTANT[RegisterUpcomingConstant(self.State)] >> STRING >> OPENPARENS[RegisterUpcomingStringVariable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> PassedParameter[RegisterVariableValue(self.State)] >> CLOSEPARENS

					| !CONSTANT[RegisterUpcomingConstant(self.State)] >> BOOLEAN >> OPENPARENS[RegisterUpcomingBooleanVariable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> PassedParameter[RegisterVariableValue(self.State)] >> CLOSEPARENS

					| !CONSTANT[RegisterUpcomingConstant(self.State)] >> REAL >> OPENPARENS[RegisterUpcomingRealVariable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> PassedParameter[RegisterVariableValue(self.State)] >> CLOSEPARENS

					| !CONSTANT[RegisterUpcomingConstant(self.State)] >> BUFFER >> OPENPARENS[RegisterUpcomingBufferVariable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> PassedParameter[RegisterVariableValue(self.State)] >> CLOSEPARENS

					| !CONSTANT[RegisterUpcomingConstant(self.State)] >> LIST >> OPENPARENS[RegisterUpcomingListVariable(self.State)][StartCountingParams(self.State)]
						>> StringIdentifier[RegisterVariableName(self.State)]
						>> COMMA >> (TypeKeywords[RegisterListType(self.State)] | OperationParameter) >> CLOSEPARENS[RegisterListVariable(self.State)]
					;

				TupleDefinition
					= TUPLE >> StringIdentifier >> COLON >> OPENPARENS
						>>  (
								(INTEGER >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (INTEGER16 >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (REAL >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (BOOLEAN >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (STRING >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							) % COMMA
					  >> CLOSEPARENS
					;

				StructureDefinition
					= STRUCTURE >> StringIdentifier >> COLON >> OPENPARENS
						>>  (
								(INTEGER >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (INTEGER16 >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (REAL >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (BOOLEAN >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | (STRING >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							  | ((StringIdentifier - TypeKeywords) >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							) % COMMA
					  >> CLOSEPARENS
					;


				HigherOrderFunctionHelper
					= (FUNCTION >> StringIdentifier >> COLON
								>> ((OPENPARENS >> (((INTEGER | INTEGER16 | STRING | BOOLEAN | REAL | StringIdentifier) >> !REFERENCE) % COMMA) >> CLOSEPARENS)
								 |  (OPENPARENS >> CLOSEPARENS))
								>> FUNCTIONARROW >> ((OPENPARENS >> ((INTEGER | INTEGER16 | STRING | BOOLEAN | REAL | StringIdentifier) % COMMA) >> CLOSEPARENS) | (OPENPARENS >> CLOSEPARENS)))
					;

				FunctionDefinition
					=
					(
						(!INFIXDECL) >>
						(StringIdentifier - EXTERNAL - TUPLE - STRUCTURE - FUNCTION - GLOBAL - INFIXDECL)[RegisterFunction(self.State)] >>
							(
								COLON >>
								((OPENPARENS >>
										(((INTEGER | INTEGER16 | STRING | BOOLEAN | REAL | (StringIdentifier - FUNCTION)) >> !REFERENCE >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
										| HigherOrderFunctionHelper
										) % COMMA >>
								CLOSEPARENS)
								| (OPENPARENS >> CLOSEPARENS))
								  >> FUNCTIONARROW
								  >> (
										(
											OPENPARENS >> 
												(  (INTEGER >> OPENPARENS >> StringIdentifier >> COMMA >> IntegerLiteral >> CLOSEPARENS)
												 | (INTEGER16 >> OPENPARENS >> StringIdentifier >> COMMA >> IntegerLiteral >> CLOSEPARENS)
												 | (STRING >> OPENPARENS >> StringIdentifier >> COMMA >> StringLiteral >> CLOSEPARENS)
												 | (BOOLEAN >> OPENPARENS >> StringIdentifier >> COMMA >> BooleanLiteral >> CLOSEPARENS)
												 | (REAL >> OPENPARENS >> StringIdentifier >> COMMA >> RealLiteral >> CLOSEPARENS)
												 | ((StringIdentifier - FUNCTION) >> OPENPARENS >> StringIdentifier[StartCountingParams(self.State)][PushIdentifierNoStack(self.State)] >> COMMA >> (OperationParameter)[FinishReturnConstructor(self.State)] >> CLOSEPARENS)
												) % COMMA
											>> CLOSEPARENS
										)
									  |
										(OPENPARENS >> CLOSEPARENS)
									)
							) >> CodeBlock
					)
					;

				ExternalDeclaration
					= EXTERNAL >> StringLiteral >> StringIdentifier >> COLON
					  >> OPENPARENS >> 
							(!(
								  ((INTEGER | INTEGER16 | STRING | BOOLEAN | REAL) >> !(REFERENCE) >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							    | (HigherOrderFunctionHelper)
								| (StringIdentifier >> !(REFERENCE) >> OPENPARENS >> StringIdentifier >> CLOSEPARENS)
							)
							% COMMA)
						>> CLOSEPARENS
					  >> FUNCTIONARROW
					  >> ((OPENPARENS >> (INTEGER | INTEGER16 | STRING | BOOLEAN | REAL) >> CLOSEPARENS) | (OPENPARENS >> CLOSEPARENS))
					;

				LibraryImport
					= LIBRARY >> OPENPARENS >> StringLiteral >> CLOSEPARENS
					;

				ExtensionImport
					= EXTENSION >> OPENPARENS >> StringLiteral >> CLOSEPARENS
					;

				GlobalBlock
					= GLOBAL >> OPENBRACE[EnterGlobalBlock(self.State)] >>
					  (
						*(VariableDefinition | TupleDefinition | StructureDefinition | Operation[RegisterOperation(self.State)])
					  )
					  >> CLOSEBRACE[ExitGlobalBlock(self.State)]
					;

				Program
					= *(ExternalDeclaration | LibraryImport | ExtensionImport | GlobalBlock | HigherOrderFunctionHelper | TupleDefinition | StructureDefinition | AliasDeclaration | FunctionDefinition)
					;

				AliasDeclaration
					= ALIASDECL >> OPENPARENS >> StringIdentifier >> COMMA >> StringIdentifier >> CLOSEPARENS
					;

				// Note that we do NOT define Assign as an infix operator here; the grammar
				// handles assignments separately in order to ensure correct semantics.
				// See the InfixAssignmentHelper production for details. The member access
				// operator is also excluded from this list for similar reasons.
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
							  ;

				self.State.SetUpUserInfixOps(*this);
				self.State.SetUpTypeAliases(*this, self.State);

				Extensions::EnumerateExtensionKeywords(RegisterEnumeratedExtension<definition<ScannerType> >(*this));

#ifdef BOOST_SPIRIT_DEBUG
				BOOST_SPIRIT_DEBUG_RULE(StringIdentifier);
				BOOST_SPIRIT_DEBUG_RULE(FunctionDefinition);
				BOOST_SPIRIT_DEBUG_RULE(PassedParameter);
				BOOST_SPIRIT_DEBUG_RULE(OperationParameter);
				BOOST_SPIRIT_DEBUG_RULE(Operation);
				BOOST_SPIRIT_DEBUG_RULE(CodeBlock);
				BOOST_SPIRIT_DEBUG_RULE(Program);

				BOOST_SPIRIT_DEBUG_RULE(LiteralValue);
				BOOST_SPIRIT_DEBUG_RULE(IntegerLiteral);
				BOOST_SPIRIT_DEBUG_RULE(StringLiteral);
				BOOST_SPIRIT_DEBUG_RULE(Control);
				BOOST_SPIRIT_DEBUG_RULE(ControlSimple);
				BOOST_SPIRIT_DEBUG_RULE(ControlWithEnding);
				BOOST_SPIRIT_DEBUG_RULE(LibraryImport);

				BOOST_SPIRIT_DEBUG_RULE(ControlKeywords);
				BOOST_SPIRIT_DEBUG_RULE(TypeKeywords);
				BOOST_SPIRIT_DEBUG_RULE(BooleanLiteral);
				BOOST_SPIRIT_DEBUG_RULE(CodeBlockContents);
				BOOST_SPIRIT_DEBUG_RULE(GlobalBlock);
				BOOST_SPIRIT_DEBUG_RULE(InfixHelper);

				BOOST_SPIRIT_DEBUG_RULE(ExternalDeclaration);
				BOOST_SPIRIT_DEBUG_RULE(RealLiteral);
				BOOST_SPIRIT_DEBUG_RULE(OtherKeywords);
				BOOST_SPIRIT_DEBUG_RULE(TupleDefinition);
				BOOST_SPIRIT_DEBUG_RULE(StructureDefinition);
				BOOST_SPIRIT_DEBUG_RULE(HigherOrderFunctionHelper);

				BOOST_SPIRIT_DEBUG_RULE(ReadStructureHelper);
				BOOST_SPIRIT_DEBUG_RULE(WriteStructureHelper);
				BOOST_SPIRIT_DEBUG_RULE(MemberHelper);
				BOOST_SPIRIT_DEBUG_RULE(HexLiteral);
				BOOST_SPIRIT_DEBUG_RULE(Task);
				BOOST_SPIRIT_DEBUG_RULE(MessageHelper);
				BOOST_SPIRIT_DEBUG_RULE(AcceptMessageHelper);

				BOOST_SPIRIT_DEBUG_RULE(ResponseMapHelper);
				BOOST_SPIRIT_DEBUG_RULE(MessageDispatch);
				BOOST_SPIRIT_DEBUG_RULE(PassedParameterBase);

				BOOST_SPIRIT_DEBUG_RULE(PassedParameterInfixList);
				BOOST_SPIRIT_DEBUG_RULE(InfixAssignmentHelper);
				BOOST_SPIRIT_DEBUG_RULE(AliasDeclaration);
				BOOST_SPIRIT_DEBUG_RULE(IncrementDecrementHelper);
				BOOST_SPIRIT_DEBUG_RULE(OpAssignmentHelper);

				BOOST_SPIRIT_DEBUG_RULE(LanguageExtensionBlock);
				BOOST_SPIRIT_DEBUG_RULE(ExtensionImport);

				BOOST_SPIRIT_DEBUG_RULE(InfixOperator);
				BOOST_SPIRIT_DEBUG_RULE(VariableDefinition);
				BOOST_SPIRIT_DEBUG_RULE(UserDefinedTypeAliases);
				BOOST_SPIRIT_DEBUG_RULE(LanguageExtensionKeywords);
#endif
			}

			// Character tokens
			boost::spirit::classic::chlit<> COLON, OPENPARENS, CLOSEPARENS, OPENBRACE, CLOSEBRACE, COMMA, QUOTE, NEGATE;

			// String tokens
			boost::spirit::classic::strlit<> FUNCTIONARROW, IF, WHILE, ELSE, DO, INTEGER, STRING, BOOLEAN, REAL, TRUETOKEN, FALSETOKEN, EXTERNAL, CAST, ASSIGN, LIBRARY, ELSEIF;
			boost::spirit::classic::strlit<> TUPLE, READTUPLE, WRITETUPLE, STRUCTURE, READSTRUCTURE, WRITESTRUCTURE, SIZEOF, INTEGER16, REFERENCE, FUNCTION, LENGTH, GLOBAL, MEMBER;
			boost::spirit::classic::strlit<> CONSTANT, HEXPREFIX, TASK, MESSAGE, ACCEPTMESSAGE, NULLFUNCTIONARROW, CALLER, SENDER, RESPONSEMAP, INFIXDECL, CRASHPARSER, FUTURE;
			boost::spirit::classic::strlit<> LIST, MAP, REDUCE, VAR, NOT, BUFFER, ALIASDECL, ADDASSIGN, SUBTRACTASSIGN, MULTIPLYASSIGN, DIVIDEASSIGN, INCREMENT, DECREMENT;
			boost::spirit::classic::strlit<> CONCATASSIGN, MEMBEROPERATOR, EXTENSION, THREAD, THREADPOOL;

			// Parser rules
			boost::spirit::classic::rule<ScannerType> StringIdentifier, FunctionDefinition, PassedParameter, OperationParameter, Operation, CodeBlock, Program;
			boost::spirit::classic::rule<ScannerType> LiteralValue, IntegerLiteral, StringLiteral, Control, ControlSimple, ControlWithEnding, LibraryImport;
			boost::spirit::classic::rule<ScannerType> ControlKeywords, TypeKeywords, BooleanLiteral, CodeBlockContents, GlobalBlock, InfixHelper;
			boost::spirit::classic::rule<ScannerType> ExternalDeclaration, RealLiteral, OtherKeywords, TupleDefinition, StructureDefinition, HigherOrderFunctionHelper;
			boost::spirit::classic::rule<ScannerType> ReadStructureHelper, WriteStructureHelper, MemberHelper, HexLiteral, Task, MessageHelper, AcceptMessageHelper;
			boost::spirit::classic::rule<ScannerType> ResponseMapHelper, MessageDispatch, PassedParameterBase, Thread, ThreadPool;
			boost::spirit::classic::rule<ScannerType> PassedParameterInfixList, InfixAssignmentHelper, AliasDeclaration, IncrementDecrementHelper, OpAssignmentHelper;
			boost::spirit::classic::rule<ScannerType> LanguageExtensionBlock, ExtensionImport;

			// Dynamic parser rules
			boost::spirit::classic::stored_rule<ScannerType> InfixOperator, VariableDefinition, UserDefinedTypeAliases, LanguageExtensionKeywords;


			//
			// Retrieve the parser's starting (root) symbol
			//
			const boost::spirit::classic::rule<ScannerType>& start() const
			{ return Program; }


			//
			// Helper for setting up user-defined infix operators
			//
			void AddUserInfixOp(const std::string& opname)
			{
				InfixOperator = InfixOperator.copy() | boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(opname.c_str()));
			}

			//
			// Helper for setting up type aliases
			//
			void AddTypeAlias(const std::string& aliasname, VM::EpochVariableTypeID type, ParserState& state)
			{
				if(type == VM::EpochVariableType_Integer)
				{
					VariableDefinition  = VariableDefinition.copy()
										| !CONSTANT[RegisterUpcomingConstant(state)]
											>> boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(aliasname.c_str()))[RegisterUpcomingIntegerVariable(state)][StartCountingParams(state)]
											>> OPENPARENS >> StringIdentifier[RegisterVariableName(state)]
											>> COMMA >> PassedParameter[RegisterVariableValue(state)] >> CLOSEPARENS
										;
				}
				else if(type == VM::EpochVariableType_Integer16)
				{
					VariableDefinition  = VariableDefinition.copy()
										| !CONSTANT[RegisterUpcomingConstant(state)]
											>> boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(aliasname.c_str()))[RegisterUpcomingInt16Variable(state)][StartCountingParams(state)]
											>> OPENPARENS >> StringIdentifier[RegisterVariableName(state)]
											>> COMMA >> PassedParameter[RegisterVariableValue(state)] >> CLOSEPARENS
										;
				}
				else if(type == VM::EpochVariableType_String)
				{
					VariableDefinition  = VariableDefinition.copy()
										| !CONSTANT[RegisterUpcomingConstant(state)]
											>> boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(aliasname.c_str()))[RegisterUpcomingStringVariable(state)][StartCountingParams(state)]
											>> OPENPARENS >> StringIdentifier[RegisterVariableName(state)]
											>> COMMA >> PassedParameter[RegisterVariableValue(state)] >> CLOSEPARENS
										;
				}
				else if(type == VM::EpochVariableType_Buffer)
				{
					VariableDefinition  = VariableDefinition.copy()
										| !CONSTANT[RegisterUpcomingConstant(state)]
											>> boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(aliasname.c_str()))[RegisterUpcomingBufferVariable(state)][StartCountingParams(state)]
											>> OPENPARENS >> StringIdentifier[RegisterVariableName(state)]
											>> COMMA >> PassedParameter[RegisterVariableValue(state)] >> CLOSEPARENS
										;
				}
				else if(type == VM::EpochVariableType_Boolean)
				{
					VariableDefinition  = VariableDefinition.copy()
										| !CONSTANT[RegisterUpcomingConstant(state)]
											>> boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(aliasname.c_str()))[RegisterUpcomingBooleanVariable(state)][StartCountingParams(state)]
											>> OPENPARENS >> StringIdentifier[RegisterVariableName(state)]
											>> COMMA >> PassedParameter[RegisterVariableValue(state)] >> CLOSEPARENS
										;
				}
				else if(type == VM::EpochVariableType_Real)
				{
					VariableDefinition  = VariableDefinition.copy()
										| !CONSTANT[RegisterUpcomingConstant(state)]
											>> boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(aliasname.c_str()))[RegisterUpcomingRealVariable(state)][StartCountingParams(state)]
											>> OPENPARENS >> StringIdentifier[RegisterVariableName(state)]
											>> COMMA >> PassedParameter[RegisterVariableValue(state)] >> CLOSEPARENS
										;
				}
				else
					throw ParserFailureException("Invalid type alias");

				UserDefinedTypeAliases = UserDefinedTypeAliases.copy() | boost::spirit::classic::strlit<>(Keywords::GetNarrowedKeyword(aliasname.c_str()));
			}

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

