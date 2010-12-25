//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper for handling semantic actions invoked by the parser
//
// Semantic actions are processed in two phases, one "pre-pass" phase and one
// compilation phase. During the pre-pass phase, information on lexical scopes,
// their contents, and general identifier usage is gathered. This data is then
// used to construct the scope metadata tables and other metadata used by the
// compilation phase itself, which actually generates bytecode from the source,
// using the pre-pass metadata for validation, type checking, and so on.
//

#include "pch.h"

#include "Compiler/SemanticActions.h"
#include "Compiler/ByteCodeEmitter.h"

#include "Metadata/FunctionSignature.h"
#include "Metadata/Precedences.h"

#include "Utility/Strings.h"
#include "Utility/NoDupeMap.h"

#include "Utility/Files/FilesAndPaths.h"

#include <sstream>
#include <iostream>
#include <limits>



//-------------------------------------------------------------------------------
// Storage of parsed tokens
//-------------------------------------------------------------------------------

//
// Store a temporary string in the temporary string slot
//
// Often we must pass over the same tokens multiple times in the course of determining
// what production rules a given token set matches. In order to prevent false positives,
// we allow the parser to store a temporary token in a special slot for later retrieval
// once the correct production is matched.
//
void CompilationSemantics::StoreTemporaryString(const std::wstring& str)
{
	TemporaryString = str;
}

//
// Shift a string token onto the general string stack
//
// Tokens can represent anything from function calls to flow control keywords to
// variable accesses; the specific significance of a string token is derived from
// its context within the code. As a result the general string stack is used for
// a wide variety of purposes, and accessed throughout the semantic action code.
//
void CompilationSemantics::StoreString(const std::wstring& name)
{
	Strings.push(name);
	PushedItemTypes.push(ITEMTYPE_STRING);
}

//
// Shift an integer literal onto the integer literal stack
//
void CompilationSemantics::StoreIntegerLiteral(Integer32 value)
{
	IntegerLiterals.push(value);
	PushedItemTypes.push(ITEMTYPE_INTEGERLITERAL);
}

//
// Shift a string literal onto the string literal stack
//
void CompilationSemantics::StoreStringLiteral(const std::wstring& value)
{
	StringLiterals.push(Session.StringPool.Pool(value));
	PushedItemTypes.push(ITEMTYPE_STRINGLITERAL);
}

//
// Shift a boolean literal onto the boolean literal stack
//
void CompilationSemantics::StoreBooleanLiteral(bool value)
{
	BooleanLiterals.push(value);
	PushedItemTypes.push(ITEMTYPE_BOOLEANLITERAL);
}

//
// Shift a real literal onto the real literal stack
//
void CompilationSemantics::StoreRealLiteral(Real32 value)
{
	RealLiterals.push(value);
	PushedItemTypes.push(ITEMTYPE_REALLITERAL);
}

//
// Shift an entity type tag onto the entity type tag stack
//
// Entity type tags are used to help generate prologue and epilogue code for each entity,
// as well as track boundaries between execution contexts for entities that trigger them
// (such as hardware acceleration, multithreading, etc.).
//
void CompilationSemantics::StoreEntityType(Bytecode::EntityTag typetag)
{
	switch(typetag)
	{
	//
	// The entity is a function. In this case, we need to track the
	// identifier of the function itself, and if applicable generate
	// the prologue code during the compilation pass.
	//
	case Bytecode::EntityTags::Function:
		CurrentEntities.push(Strings.top());
		Strings.pop();
		PushedItemTypes.pop();
		if(!IsPrepass)
			EmitterStack.top()->EnterFunction(LexicalScopeStack.top());
		break;

	//
	// The entity is a free-standing block. We need to generate an
	// anonymous name for this block so we can refer to its scope
	// elsewhere in the compilation process. We also need to manually
	// generate a lexical scope and emit some prologue code.
	//
	case Bytecode::EntityTags::FreeBlock:
		{
			std::wstring anonymousname = AllocateAnonymousScopeName();
			StringHandle anonymousnamehandle = Session.StringPool.Pool(anonymousname);
			CurrentEntities.push(anonymousname);
			LexicalScopeDescriptions.insert(std::make_pair(anonymousnamehandle, ScopeDescription(&LexicalScopeDescriptions.find(LexicalScopeStack.top())->second)));
			LexicalScopeStack.push(anonymousnamehandle);
			if(!IsPrepass)
				EmitterStack.top()->EnterEntity(Bytecode::EntityTags::FreeBlock, anonymousnamehandle);
		}
		break;

	//
	// Handle the error case where the type tag is not recognized. This
	// generally indicates a catastrophic failure in the parser setup,
	// because custom entities are handled separately; so we throw a
	// fatal exception.
	//
	default:
		throw FatalException("Invalid entity type tag");
	}

	// Push the tag onto the list now that it is known to be valid
	EntityTypeTags.push(typetag);
}

//
// Shift a custom entity type tag onto the entity type tag stack
//
// Custom entities are used to implement things like flow control, threading, and so on.
// A custom entity attaches a set of parameters (similar to a function's parameters) to
// a code block.
//
void CompilationSemantics::StoreEntityType(const std::wstring& identifier)
{
	StringHandle handle = Session.StringPool.Pool(identifier);
	EntityTypeTags.push(LookupEntityTag(handle));

	TemporaryString = identifier;
}

//
// Store the postfix expression for a postfix entity
//
// Postfix entities can accept optional parameters to an expression which follows the
// entity's code block body; this is useful for implementing certain types of loops,
// such as do/while.
//
void CompilationSemantics::StoreEntityPostfix(const std::wstring& identifier)
{
	StringHandle handle = Session.StringPool.Pool(identifier);
	EntityTypeTags.push(LookupEntityTag(handle));

	TemporaryString = identifier;
}

//
// Register that we have ended the closing expression of a postfix entity
//
// Our main goal here is to emit code which passes off the expression's value to the appropriate
// entity meta-control code, so that the entity's flow control will be performed correctly.
//
void CompilationSemantics::InvokePostfixMetacontrol()
{
	if(!IsPrepass)
	{
		EmitterStack.top()->InvokeMetacontrol(EntityTypeTags.top());
		StatementTypes.pop();
	}

	EntityTypeTags.pop();

	// We also need to store off the code generated by the entity
	StoreEntityCode();
}

//
// Flag the end of an entity code block and update parser state accordingly
//
// This routine emits any necessary epilogue code for the entity, as well as shifts the internal
// state so that the current lexical scope and entity metadata are cleaned up and we return to
// the original parent context. In general this should be kept as flexible as possible so that
// we can support features like inner functions, nested anonymous scopes, and so on.
//
void CompilationSemantics::StoreEntityCode()
{
	if(!IsPrepass)
	{
		switch(EntityTypeTags.top())
		{
		//
		// The entity being processed is a function. In this case we need to generate some epilogue
		// code to handle return values for functions that are not void. Should we fail to look
		// up the function signature of the entity we are exiting, we must raise a fatal error.
		//
		case Bytecode::EntityTags::Function:
			{
				FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(LexicalScopeStack.top());
				if(iter == Session.FunctionSignatures.end())
					throw FatalException("Failed to locate function being finalized");

				if(iter->second.GetReturnType() != VM::EpochType_Void)
				{
					const ScopeDescription& scope = LexicalScopeDescriptions[LexicalScopeStack.top()];

					size_t retindex = std::numeric_limits<size_t>::max();
					for(size_t i = 0; i < scope.GetVariableCount(); ++i)
					{
						if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_RETURN)
						{
							retindex = i;
							break;
						}
					}
					if(retindex >= scope.GetVariableCount())
						throw FatalException("Function is not void but the return variable could not be located");

					StringHandle retnamehandle = scope.GetVariableNameHandle(retindex);

					if(ConstructorNames.find(LexicalScopeStack.top()) != ConstructorNames.end())
					{
						size_t idparamindex = std::numeric_limits<size_t>::max();
						for(size_t i = 0; i < scope.GetVariableCount(); ++i)
						{
							if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_PARAMETER && scope.GetVariableTypeByIndex(i) == VM::EpochType_Identifier)
							{
								idparamindex = i;
								break;
							}
						}
						if(idparamindex >= scope.GetVariableCount())
							throw FatalException("Constructors must accept an identifier parameter as the first parameter");

						StringHandle idparamnamehandle = scope.GetVariableNameHandle(idparamindex);

						EmitterStack.top()->PushVariableValueNoCopy(retnamehandle);
						EmitterStack.top()->PushVariableValue(idparamnamehandle, VM::EpochType_Identifier);
						EmitterStack.top()->PushIntegerLiteral(iter->second.GetReturnType());
						EmitterStack.top()->AssignVariableThroughIdentifier();
					}

					EmitterStack.top()->SetReturnRegister(retnamehandle);
				}

				EmitterStack.top()->ExitFunction();
			}
			break;

		//
		// The entity being processed is a free-standing code block. There is no special epilogue
		// code needed; the entity exit instruction will cause the virtual machine to clean up the
		// lexical scope as appropriate.
		//
		case Bytecode::EntityTags::FreeBlock:
			EmitterStack.top()->ExitEntity();
			break;

		//
		// Error case: the entity tag is not recognized.
		//
		case Bytecode::EntityTags::Invalid:
			throw FatalException("Invalid entity type tag");

		//
		// The entity tag must be a custom entity. We assume that no special epilogue code will be
		// required, since epilogues for custom entities are handled via postfix entity expressions.
		//
		default:
			EmitterStack.top()->ExitEntity();
		}
	}

	CurrentEntities.pop();
	EntityTypeTags.pop();
	LexicalScopeStack.pop();
}

//
// Store a member access reference used in certain expression types
//
// The member access operator . is a special case infix operator with syntactic significance
// to the parser. Specifically, we need to recognize its presence when performing certain
// operations such as pre-increment and so on. This function helps ensure that the correct
// sequence of member accesses is tracked, particularly for dealing with nested structures.
//
void CompilationSemantics::StoreMember(const std::wstring& member)
{
	if(!IsPrepass)
	{
		if(!TemporaryString.empty())
		{
			AssignmentTargets.push(AssignmentTarget(Session.StringPool.Pool(TemporaryString)));
			TemporaryString.clear();
		}

		AssignmentTargets.top().Members.push_back(Session.StringPool.Pool(member));
	}
}


//-------------------------------------------------------------------------------
// Infix expression handling
//-------------------------------------------------------------------------------

//
// Record the parsing of an infix operator
//
// At this point we have already parsed the first term of the infix expression (be it
// a single variable/literal or another expression), and we now have parsed the operator
// itself (given in the parameter to the function). We need to save off the identifier
// so that once we parse the second half of the expression we can use it to assemble the
// final infix expression (respecting precedence rules) and then emit the corresponding
// bytecode.
//
void CompilationSemantics::StoreInfix(const std::wstring& identifier)
{
	StatementNames.push(identifier);
	StatementParamCount.push(0);
	PushParam(L"@@infixresult");
}

//
// Record the end of an infix expression
//
// This may not be the actual end of the expression per se, but rather simply signals
// that both terms and the infix operator itself have all been parsed and saved off into
// the state data. Finalization of the expression and handling of operator precedence
// rules is delegated to the FinalizeInfix() function.
//
void CompilationSemantics::CompleteInfix()
{
	StatementParamCount.pop();

	if(!IsPrepass)
	{
		std::wstring infixstatementname = StatementNames.top();
		StringHandle infixstatementnamehandle = Session.StringPool.Pool(infixstatementname);

		InfixOperators.top().push_back(infixstatementnamehandle);
	}

	StatementNames.pop();
	PushedItemTypes.push(ITEMTYPE_STATEMENT);
}

//
// Finalize an infix expression
//
// First we invoke CollapseUnaryOperators() to reduce all unary subexpressions (e.g. of the form !foo)
// into single operands with expression-type payloads. Then, we iteratively reduce operator/operand pairings
// into single operands, with higher-precedence operators reduced first, maintaining a left-to-right order of
// evaluation. This reduction results in the entire infix expression being simplified to a single compile
// time parameter value with an expression payload that equates to the infix expression being finalized.
//
void CompilationSemantics::FinalizeInfix()
{
	if(!IsPrepass)
	{
		CollapseUnaryOperators();

		if(!InfixOperators.empty() && !InfixOperators.top().empty())
		{
			CompileTimeParameterVector flatoperandlist;

			// Store all operands which belong to this infix expression; take care to
			// skip earlier operands, which are parameters to the statement currently
			// being parsed, and not part of the expression itself.
			for(CompileTimeParameterVector::const_iterator operanditer = CompileTimeParameters.top().begin() + StatementParamCount.top(); operanditer != CompileTimeParameters.top().end(); ++operanditer)
				flatoperandlist.push_back(*operanditer);

			if(!flatoperandlist.empty())
			{
				size_t numoperandscollapsed = flatoperandlist.size();

				// Iterate through the list of known operator precedences, highest first
				for(PrecedenceTable::const_reverse_iterator precedenceiter = Session.OperatorPrecedences.rbegin(); precedenceiter != Session.OperatorPrecedences.rend(); ++precedenceiter)
				{
					// We must note if we are handling the magic . member access operator, because
					// if so, we should not evaluate the identifiers given to us, but rather pass
					// them along raw to the . operator overload in question. For instance, in the
					// expression foo.bar, we don't want to evaluate foo OR bar, but rather pass
					// the two identifiers "foo" and "bar" to the . operator for final evaluation.
					// Note that it may be possible (indeed, even desirable) to make this a more
					// general mechanism, whereby the operator overload's parameters are checked to
					// see if the overload expects identifiers; but for now this hack will suffice.
					bool ismemberaccess = (precedenceiter->first == PRECEDENCE_MEMBERACCESS);

					// Repeatedly reduce expressions to single operands, until no more operators
					// at this precedence level are encountered. This ensures that sequences of
					// operations with the same precedence are all reduced simultaneously.
					bool collapsed;
					do
					{
						collapsed = false;

						CompileTimeParameterVector::iterator operanditer = flatoperandlist.begin();
						for(StringHandles::iterator unmappedoperatoriter = InfixOperators.top().begin(); unmappedoperatoriter != InfixOperators.top().end(); ++unmappedoperatoriter)
						{
							// Note here that we look up predecenes based on the operator symbol itself, not the
							// resolved overload of the operator. The result is that all operator overloads will
							// have the same precedence (which is nice for consistency). This also means that we
							// don't have to manually define a precedence for all the . operator overloads which
							// get generated for structure member accesses.
							if(GetOperatorPrecedence(*unmappedoperatoriter) == precedenceiter->first)
							{
								StringHandle infixstatementnamehandle = *unmappedoperatoriter;
								std::wstring infixstatementname = Session.StringPool.GetPooledString(infixstatementnamehandle);
								RemapFunctionToOverload(flatoperandlist, operanditer - flatoperandlist.begin(), 2, 2, TypeVector(), infixstatementname, infixstatementnamehandle);

								FunctionSignatureSet::const_iterator funcsigiter = Session.FunctionSignatures.find(infixstatementnamehandle);
								if(funcsigiter == Session.FunctionSignatures.end())
									throw FatalException("Unknown statement, cannot complete parsing");

								// Perform the actual reduction, taking care to clean up the
								// list of pending operators as we eliminate subexpressions.
								ByteBuffer buffer;
								ByteCodeEmitter emitter(buffer);

								CompileTimeParameterVector::iterator firstoperanditer = operanditer;

								VM::EpochTypeID op1type = GetEffectiveType(*operanditer);
								if(ismemberaccess)
									emitter.PushVariableValueNoCopy(operanditer->Payload.StringHandleValue);
								else
									EmitInfixOperand(emitter, *operanditer);

								++operanditer;

								VM::EpochTypeID op2type = ismemberaccess ? VM::EpochType_Identifier : GetEffectiveType(*operanditer);
								if(ismemberaccess)
									emitter.PushStringLiteral(operanditer->Payload.StringHandleValue);
								else
									EmitInfixOperand(emitter, *operanditer);

								CompileTimeParameterVector::iterator secondoperanditer = operanditer;
								++operanditer;
								emitter.Invoke(infixstatementnamehandle);

								VerifyInfixOperandTypes(infixstatementnamehandle, op1type, op2type);

								firstoperanditer->Type = VM::EpochType_Expression;
								firstoperanditer->ExpressionType = Session.FunctionSignatures.find(infixstatementnamehandle)->second.GetReturnType();
								firstoperanditer->ExpressionContents.swap(buffer);

								flatoperandlist.erase(secondoperanditer);
								InfixOperators.top().erase(unmappedoperatoriter);

								collapsed = true;
								break;
							}
							else
								++operanditer;
						}
					} while(collapsed);
				}

				// Erase the operands that were reduced
				CompileTimeParameters.top().erase(CompileTimeParameters.top().end() - numoperandscollapsed, CompileTimeParameters.top().end());

				// Shift the final expression onto the compile time parameter list
				CompileTimeParameter ctparam(L"@@infixresult", VM::EpochType_Expression);
				ctparam.ExpressionType = flatoperandlist[0].ExpressionType;
				ctparam.ExpressionContents.swap(flatoperandlist[0].ExpressionContents);
				CompileTimeParameters.top().push_back(ctparam);
			}
		}
	}

	if(!StatementParamCount.empty())
		++StatementParamCount.top();
}

//
// Register the beginning of a parenthetical expression
//
void CompilationSemantics::BeginParenthetical()
{
	if(!IsPrepass)
	{
		InfixOperators.push(StringHandles());
		UnaryOperators.push(UnaryOperatorVector());
		CompileTimeParameters.push(CompileTimeParameterVector());
		StatementParamCount.push(0);
	}
}

//
// Register the end of a parenthetical expression
//
// We need to do some minor housekeeping to ensure that infix operands are in the right order and such.
//
void CompilationSemantics::EndParenthetical()
{
	if(!IsPrepass)
	{
		CompileTimeParameter ctparam = CompileTimeParameters.top().back();
		InfixOperators.pop();
		UnaryOperators.pop();
		CompileTimeParameters.pop();
		CompileTimeParameters.top().push_back(ctparam);
		StatementParamCount.pop();
	}
}


//-------------------------------------------------------------------------------
// Unary and pre/post-operators
//-------------------------------------------------------------------------------

//
// Register a unary prefix operator
//
// These operators have no side effects; they simply perform a unary computation.
//
void CompilationSemantics::StoreUnaryPrefixOperator(const std::wstring& identifier)
{
	if(!IsPrepass)
		UnaryOperators.top().push_back(std::make_pair(Session.StringPool.Pool(identifier), CompileTimeParameters.top().size()));
}

//
// Collapse unary operators into flat expressions
//
// By definition unary operators always affect the expression immediately to their right;
// this is used to pair off operators and operands in the expression stack, and collapse
// them into single operands that have an expression payload equivalent to the right hand
// side of the expression under the unary operation.
//
void CompilationSemantics::CollapseUnaryOperators()
{
	if(!UnaryOperators.empty() && !UnaryOperators.top().empty())
	{
		unsigned numcollapsed = 0;
		for(UnaryOperatorVector::const_iterator iter = UnaryOperators.top().begin(); iter != UnaryOperators.top().end(); ++iter)
		{
			ByteBuffer buffer;
			ByteCodeEmitter emitter(buffer);

			size_t ctparamindex = iter->second;
			if(ctparamindex >= CompileTimeParameters.top().size())
				break;

			const CompileTimeParameter& originalparam = CompileTimeParameters.top()[ctparamindex];
			EmitInfixOperand(emitter, originalparam);
			
			StringHandle outhandle = iter->first;
			std::wstring outname = Session.StringPool.GetPooledString(outhandle);
			CompileTimeParameterVector paramvec(1, originalparam);
			RemapFunctionToOverload(paramvec, 0, 1, std::numeric_limits<size_t>::max(), TypeVector(), outname, outhandle);
			emitter.Invoke(outhandle);

			CompileTimeParameter ctparam(L"@@unaryresult", VM::EpochType_Expression);
			ctparam.ExpressionType = GetEffectiveType(originalparam);
			ctparam.ExpressionContents.swap(buffer);
			CompileTimeParameters.top()[ctparamindex] = ctparam;

			++numcollapsed;
		}

		if(numcollapsed)
			UnaryOperators.top().erase(UnaryOperators.top().begin(), UnaryOperators.top().begin() + numcollapsed);
	}
}


//
// Register the operator of a pre-operator expression
//
// We simply save the operator's identifier in a temporary string slot, since
// it is possible that we might fail to find a valid operand, in which case we
// don't want to garbage up the parser state with bogus data. Note that we
// can't use "the" temporary string slot, because we might need that for other
// purposes, such as member access operators and parentheticals.
//
void CompilationSemantics::RegisterPreOperator(const std::wstring& identifier)
{
	if(!IsPrepass)
		PreOperatorString = identifier;
}

//
// Register the operand of a pre-operator expression
//
// At this point we have both an operator and an operand, so we can successfully
// check for type validity and emit any necessary code.
//
void CompilationSemantics::RegisterPreOperand(const std::wstring& expression)
{
	if(!IsPrepass)
	{
		std::wstring operatorname = PreOperatorString;
		StringHandle operatornamehandle = Session.StringPool.Pool(operatorname);

		if(!TemporaryString.empty())
		{
			AssignmentTargets.push(AssignmentTarget(Session.StringPool.Pool(TemporaryString)));
			TemporaryString.clear();
		}

		VM::EpochTypeID variabletype = GetEffectiveType(AssignmentTargets.top());

		RemapFunctionToOverload(CompileTimeParameterVector(), 0, 1, 1, TypeVector(1, variabletype), operatorname, operatornamehandle);

		ByteBuffer buffer;
		ByteCodeEmitter emitter(buffer);

		AssignmentTargets.top().EmitCurrentValue(emitter, LexicalScopeDescriptions.find(LexicalScopeStack.top())->second, Structures, StructureNames, Session.StringPool);
		emitter.Invoke(operatornamehandle);
		AssignmentTargets.top().EmitReferenceBindings(emitter);
		emitter.AssignVariable();
		AssignmentTargets.top().EmitCurrentValue(emitter, LexicalScopeDescriptions.find(LexicalScopeStack.top())->second, Structures, StructureNames, Session.StringPool);

		StatementTypes.push(variabletype);

		if(!CompileTimeParameters.empty())
		{
			CompileTimeParameter ctparam(L"@@preoperation", VM::EpochType_Expression);
			ctparam.ExpressionType = variabletype;
			ctparam.ExpressionContents.swap(buffer);
			CompileTimeParameters.top().push_back(ctparam);
		}
		else
			EmitterStack.top()->EmitBuffer(buffer);

		AssignmentTargets.pop();
	}

	PushedItemTypes.push(ITEMTYPE_STATEMENT);
}

//
// Register the operator of a post-operator expression
//
// At this point we have both an operator and an operand, so we can perform the
// necessary type validation and emit any appropriate bytecode.
//
void CompilationSemantics::RegisterPostOperator(const std::wstring& identifier)
{
	if(!IsPrepass)
	{
		std::wstring operatorname = identifier;
		StringHandle operatornamehandle = Session.StringPool.Pool(operatorname);

		if(AssignmentTargets.empty() && !TemporaryString.empty())
		{
			AssignmentTargets.push(AssignmentTarget(Session.StringPool.Pool(TemporaryString)));
			TemporaryString.clear();
		}

		VM::EpochTypeID variabletype = GetEffectiveType(AssignmentTargets.top());

		RemapFunctionToOverload(CompileTimeParameterVector(), 0, 1, 1, TypeVector(1, variabletype), operatorname, operatornamehandle);

		ByteBuffer buffer;
		ByteCodeEmitter emitter(buffer);

		// Yes, we need to push this twice! (Once, the value is passed on to the operator
		// itself for invocation; the second push [or rather the one which happens first,
		// and appears lower on the stack] is used to hold the initial value of the expression
		// so that the subsequent code can read off the value safely, in keeping with the
		// traditional semantics of a post operator.)
		AssignmentTargets.top().EmitCurrentValue(emitter, LexicalScopeDescriptions.find(LexicalScopeStack.top())->second, Structures, StructureNames, Session.StringPool);
		AssignmentTargets.top().EmitCurrentValue(emitter, LexicalScopeDescriptions.find(LexicalScopeStack.top())->second, Structures, StructureNames, Session.StringPool);
		emitter.Invoke(operatornamehandle);
		AssignmentTargets.top().EmitReferenceBindings(emitter);
		emitter.AssignVariable();

		StatementTypes.push(variabletype);

		if(!CompileTimeParameters.empty())
		{
			CompileTimeParameter ctparam(L"@@preoperation", VM::EpochType_Expression);
			ctparam.ExpressionType = variabletype;
			ctparam.ExpressionContents.swap(buffer);
			CompileTimeParameters.top().push_back(ctparam);
		}
		else
			EmitterStack.top()->EmitBuffer(buffer);

		AssignmentTargets.pop();
	}

	PushedItemTypes.push(ITEMTYPE_STATEMENT);
}


//-------------------------------------------------------------------------------
// Entity parameter definitions
//-------------------------------------------------------------------------------

//
// Begin parsing the definition list of parameters passed to an entity (usually a function)
//
// Note that this is for entity definitions only, not entity invocations.
//
void CompilationSemantics::BeginParameterSet()
{
	NeedsPatternResolver = false;
	InsideParameterList = true;
	FunctionSignatureStack.push(FunctionSignature());
	if(IsPrepass)
	{
		StringHandle overload = AllocateNewOverloadedFunctionName(Session.StringPool.Pool(Strings.top()));
		AddLexicalScope(overload);
		OverloadDefinitions.push_back(std::make_pair(ParsePosition, overload));
	}
	else
	{
		for(OverloadPositionList::const_iterator iter = OverloadDefinitions.begin(); iter != OverloadDefinitions.end(); ++iter)
		{
			if(iter->first == ParsePosition)
			{
				LexicalScopeStack.push(iter->second);
				return;
			}
		}

		throw FatalException("Lost track of a function overload definition somewhere along the line");
	}
}

//
// Finish parsing the definition list of parameters passed to an entity
//
void CompilationSemantics::EndParameterSet()
{
	InsideParameterList = false;
}

//
// Register the type of a function/entity parameter
//
void CompilationSemantics::RegisterParameterType(const std::wstring& type)
{
	ParamType = LookupTypeName(type);
	ParamIsReference = false;
}

//
// Register the name of a function/entity parameter
//
// Adds the parameter to the entity's signature, and optionally in the compilation phase
// adds the corresponding variable to the entity's local lexical scope.
//
void CompilationSemantics::RegisterParameterName(const std::wstring& name)
{
	FunctionSignatureStack.top().AddParameter(name, ParamType, ParamIsReference);
	
	if(!IsPrepass)
	{
		ScopeMap::iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
		if(iter == LexicalScopeDescriptions.end())
			throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");
		
		iter->second.AddVariable(name, Session.StringPool.Pool(name), ParamType, ParamIsReference, VARIABLE_ORIGIN_PARAMETER);
	}
}

//
// Register that a function parameter requires pattern matching
//
// Pattern matching allows function overloads to be invoked depending on the values of the function
// parameters rather than just the types.
//
void CompilationSemantics::RegisterPatternMatchedParameter()
{
	VM::EpochTypeID paramtype = VM::EpochType_Error;

	switch(PushedItemTypes.top())
	{
	case ITEMTYPE_INTEGERLITERAL:
		{
			FunctionSignatureStack.top().AddPatternMatchedParameter(IntegerLiterals.top());
			IntegerLiterals.pop();
			paramtype = VM::EpochType_Integer;
		}
		break;

	case ITEMTYPE_STRINGLITERAL:
		throw NotImplementedException("Pattern matching on this parameter type is not implemented");
		StringLiterals.pop();
		break;

	case ITEMTYPE_BOOLEANLITERAL:
		throw NotImplementedException("Pattern matching on this parameter type is not implemented");
		BooleanLiterals.pop();
		break;

	case ITEMTYPE_REALLITERAL:
		throw NotImplementedException("Pattern matching on this parameter type is not implemented");
		RealLiterals.pop();
		break;

	case ITEMTYPE_STATEMENT:
		throw NotImplementedException("Pattern matching on general expressions is not implemented");
		StatementNames.pop();
		StatementTypes.pop();
		break;

	default:
		throw NotImplementedException("Unsupported expression in pattern matched function parameter");
	}

	PushedItemTypes.pop();

	NeedsPatternResolver = true;

	if(!IsPrepass)
	{
		ScopeMap::iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
		if(iter == LexicalScopeDescriptions.end())
			throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");
		
		iter->second.AddVariable(L"@@patternmatched", Session.StringPool.Pool(L"@@patternmatched"), paramtype, false, VARIABLE_ORIGIN_PARAMETER);
	}
}

//
// Register that a parameter should have reference semantics
//
void CompilationSemantics::RegisterParameterIsReference()
{
	ParamIsReference = true;
}


//-------------------------------------------------------------------------------
// Entity return definitions
//-------------------------------------------------------------------------------

//
// Register the start of an entity's return value definition
//
// During the compilation process, we need to set up a temporary bytecode emitter and buffer,
// which receive the code generated by the expression in the initializer. Otherwise, the code
// will be emitted at the scope above the entity being defined, which is incorrect. Since the
// code needs access to the local variables defined by the entity's parameters, the code must
// be within the entity itself via some prologue code; this also ensures that the initializer
// is called correctly every time the entity is invoked. To enforce this we redirect all code
// emission to a temporary buffer and then flush that buffer into the parent stream after all
// prologue code has been generated.
//
void CompilationSemantics::BeginReturnSet()
{
	if(!IsPrepass)
	{
		PendingEmissionBuffers.push(ByteBuffer());
		PendingEmitters.push(ByteCodeEmitter(PendingEmissionBuffers.top()));
		EmitterStack.push(&PendingEmitters.top());
	}
	ParsingReturnDeclaration = true;
}

//
// End the definition of an entity's return value
//
// This routine's responsibilities are two-fold. During the pre-pass phase, it handles
// adding the entity's parameter/return signature to the appropriate scope. During the
// compilation phase, it cleans up from any expression that appears in the initializer
// of the return value itself, as well as flushes the output buffer as described above
// in the remarks on BeginReturnSet.
//
void CompilationSemantics::EndReturnSet()
{
	if(IsPrepass)
	{
		Session.FunctionSignatures.insert(std::make_pair(LexicalScopeStack.top(), FunctionSignatureStack.top()));
		if(NeedsPatternResolver)
		{
			StringHandle resolvernamehandle = Session.StringPool.Pool(GetPatternMatchResolverName(Strings.top()));
			if(Session.FunctionSignatures.find(resolvernamehandle) == Session.FunctionSignatures.end())
			{
				Session.FunctionSignatures.insert(std::make_pair(resolvernamehandle, FunctionSignatureStack.top()));
				NeededPatternResolvers[resolvernamehandle] = FunctionSignatureStack.top();
			}
			OriginalFunctionsForPatternResolution.insert(std::make_pair(resolvernamehandle, LexicalScopeStack.top()));
		}
		else
		{
			// See if this is the default pattern matcher for any other pattern-matched overloads
			for(FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.begin(); iter != Session.FunctionSignatures.end(); ++iter)
			{
				if(Session.StringPool.GetPooledString(iter->first) == GetPatternMatchResolverName(Strings.top()))
				{
					OriginalFunctionsForPatternResolution.insert(std::make_pair(iter->first, LexicalScopeStack.top()));
					break;
				}
			}
		}
	}
	else
	{
		EmitterStack.pop();

		VM::EpochTypeID rettype = VM::EpochType_Void;
		const ScopeDescription& scope = LexicalScopeDescriptions.find(LexicalScopeStack.top())->second;
		for(size_t i = 0; i < scope.GetVariableCount(); ++i)
		{
			if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_RETURN)
			{
				rettype = scope.GetVariableTypeByIndex(i);
				break;
			}
		}

		if(rettype != VM::EpochType_Void)
			Session.FunctionSignatures[LexicalScopeStack.top()].SetReturnType(rettype);
	}

	FunctionSignatureStack.pop();
	ParsingReturnDeclaration = false;
}

//
// Determine if we are currently parsing a function return declaration
//
bool CompilationSemantics::IsInReturnDeclaration() const
{
	return ParsingReturnDeclaration;
}


//-------------------------------------------------------------------------------
// Statements
//-------------------------------------------------------------------------------

//
// Speculatively parse the beginning of a statement
//
// Note that we can't yet shift anything onto the parse stacks, because we may actually
// get called here multiple times per statement, as the parser itself attempts to match
// the statement to the appropriate grammar rule. Instead we simply store the passed in
// token in a temporary slot, then shift it onto the appropriate stack later when it is
// clear what grammar rule we're actually trying to handle.
//
void CompilationSemantics::BeginStatement(const std::wstring& statementname)
{
	TemporaryString = statementname;
}

//
// Begin parsing the parameters passed to a statement
//
// At this point it is clear that we are parsing a function/entity invocation statement
// (as opposed to, for example, an assignment operation) and therefore we can shift the
// temporary string token onto the statement name stack accordingly. During the compile
// phase we also need to set up storage space for tracking the parameters themselves.
//
void CompilationSemantics::BeginStatementParams()
{
	StatementNames.push(TemporaryString);
	TemporaryString.clear();
	StatementParamCount.push(0);
	if(!IsPrepass)
	{
		CompileTimeParameters.push(CompileTimeParameterVector());
		InfixOperators.push(StringHandles());
		UnaryOperators.push(UnaryOperatorVector());

		PendingEmissionBuffers.push(ByteBuffer());
		PendingEmitters.push(ByteCodeEmitter(PendingEmissionBuffers.top()));
	}
}

//
// Finish parsing a subexpression/statement
//
// Note that this may not necessarily be the outermost statement, i.e. it may be nested
// within other statements, such as in the expression "foo(bar(baz() + 42)))". See also
// the FinalizeStatement function for handling the outermost statement.
//
// The primary goal of this routine is to maintain a record of the types of each nested
// expression so that parameter validation can be performed correctly. The routine also
// emits the actual code for invoking the associated function/entity.
//
void CompilationSemantics::CompleteStatement()
{
	struct onexit_
	{
		onexit_(CompilationSemantics* p) : ThisPtr(p) {}
		~onexit_() { ThisPtr->StatementParamCount.pop(); }

		CompilationSemantics* ThisPtr;
	} onexit(this);

	std::wstring statementname = StatementNames.top();
	StringHandle statementnamehandle = Session.StringPool.Pool(statementname);

	StatementNames.pop();
	PushedItemTypes.push(ITEMTYPE_STATEMENT);

	if(!IsPrepass)
	{
		InfixOperators.pop();
		UnaryOperators.pop();

		TypeVector outerexpectedtypes = WalkCallChainForExpectedTypes(StatementNames.size() - 1);
		RemapFunctionToOverload(CompileTimeParameters.top(), 0, StatementParamCount.top(), std::numeric_limits<size_t>::max(), outerexpectedtypes, statementname, statementnamehandle);

		FunctionCompileHelperTable::const_iterator fchiter = CompileTimeHelpers.find(statementnamehandle);
		if(fchiter != CompileTimeHelpers.end())
			fchiter->second(statementname, *this, GetLexicalScopeDescription(LexicalScopeStack.top()), CompileTimeParameters.top());

		bool indirectinvoke = false;

		const FunctionSignature* fs = NULL;
		{
			FunctionSignatureSet::const_iterator fsiter = Session.FunctionSignatures.find(statementnamehandle);
			if(fsiter == Session.FunctionSignatures.end())
			{
				if(GetLexicalScopeDescription(LexicalScopeStack.top()).HasVariable(statementname) && GetLexicalScopeDescription(LexicalScopeStack.top()).GetVariableTypeByID(statementnamehandle) == VM::EpochType_Function)
				{
					const FunctionSignature& signature = Session.FunctionSignatures.find(LexicalScopeStack.top())->second;
					fs = &signature.GetFunctionSignature(signature.FindParameter(statementname));
					indirectinvoke = true;
				}
				else
					Throw(RecoverableException("The function \"" + narrow(statementname) + "\" is not defined in this scope"));
			}
			else
				fs = &fsiter->second;
		}

		// Validate the parameters we've been given against the parameters to this overload
		if(CompileTimeParameters.top().size() == fs->GetNumParameters())
		{
			for(size_t i = 0; i < CompileTimeParameters.top().size(); ++i)
			{
				switch(CompileTimeParameters.top()[i].Type)
				{
				case VM::EpochType_Error:
					throw FatalException("Parameter's type is explicitly flagged as invalid");

				case VM::EpochType_Void:
					Throw(ParameterException("Parameter has no type; cannot be passed to this function"));

				case VM::EpochType_Identifier:
					if(fs->GetParameter(i).Type == VM::EpochType_Identifier)
					{
						if(fs->GetParameter(i).IsReference)
							PendingEmitters.top().BindReference(CompileTimeParameters.top()[i].Payload.StringHandleValue);
						else
							PendingEmitters.top().PushStringLiteral(CompileTimeParameters.top()[i].Payload.StringHandleValue);
					}
					else
					{
						ScopeMap::const_iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
						if(iter == LexicalScopeDescriptions.end())
							throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");

						if(fs->GetParameter(i).Type == VM::EpochType_Function)
						{
							FunctionSignatureSet::const_iterator fsiter = Session.FunctionSignatures.find(CompileTimeParameters.top()[i].Payload.StringHandleValue);
							if(fsiter == Session.FunctionSignatures.end())
								Throw(RecoverableException("No function by the name \"" + narrow(CompileTimeParameters.top()[i].StringPayload) + "\" was found in this scope"));

							if(!fsiter->second.Matches(fs->GetFunctionSignature(i)))
								Throw(RecoverableException("No overload of \"" + narrow(CompileTimeParameters.top()[i].StringPayload) + "\" matches the function requirements"));

							PendingEmitters.top().PushStringLiteral(CompileTimeParameters.top()[i].Payload.StringHandleValue);
						}
						else
						{
							if(iter->second.HasVariable(CompileTimeParameters.top()[i].StringPayload))
							{
								if(iter->second.GetVariableTypeByID(CompileTimeParameters.top()[i].Payload.StringHandleValue) != fs->GetParameter(i).Type)
									Throw(TypeMismatchException("The variable \"" + narrow(CompileTimeParameters.top()[i].StringPayload) + "\" has the wrong type to be used for this parameter"));

								if(fs->GetParameter(i).IsReference)
									PendingEmitters.top().BindReference(CompileTimeParameters.top()[i].Payload.StringHandleValue);
								else
									PendingEmitters.top().PushVariableValue(CompileTimeParameters.top()[i].Payload.StringHandleValue, GetEffectiveType(CompileTimeParameters.top()[i]));
							}
							else
								Throw(RecoverableException("No variable by the name \"" + narrow(CompileTimeParameters.top()[i].StringPayload) + "\" was found in this scope"));
						}
					}
					break;

				case VM::EpochType_Expression:
					if(fs->GetParameter(i).Type == CompileTimeParameters.top()[i].ExpressionType)
					{
						if(fs->GetParameter(i).IsReference)
							throw(NotImplementedException("Support for reference expressions is not implemented"));
						else
							PendingEmitters.top().EmitBuffer(CompileTimeParameters.top()[i].ExpressionContents);
					}
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(TypeMismatchException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_Integer:
					if(fs->GetParameter(i).Type == VM::EpochType_Integer)
						PendingEmitters.top().PushIntegerLiteral(CompileTimeParameters.top()[i].Payload.IntegerValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(TypeMismatchException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_String:
					if(fs->GetParameter(i).Type == VM::EpochType_String)
						PendingEmitters.top().PushStringLiteral(CompileTimeParameters.top()[i].Payload.StringHandleValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(TypeMismatchException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_Boolean:
					if(fs->GetParameter(i).Type == VM::EpochType_Boolean)
						PendingEmitters.top().PushBooleanLiteral(CompileTimeParameters.top()[i].Payload.BooleanValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(TypeMismatchException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_Real:
					if(fs->GetParameter(i).Type == VM::EpochType_Real)
						PendingEmitters.top().PushRealLiteral(CompileTimeParameters.top()[i].Payload.RealValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(TypeMismatchException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_Buffer:
					if(fs->GetParameter(i).Type == VM::EpochType_Buffer)
						PendingEmitters.top().PushBufferHandle(CompileTimeParameters.top()[i].Payload.BufferHandleValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(TypeMismatchException(narrow(errormsg.str())));
					}
					break;

				default:
					throw NotImplementedException("Support for parameters of this type is not implemented");
				}
			}
		}
		else
		{
			std::wostringstream errormsg;
			errormsg << L"The function \"" << statementname << L"\" expects " << fs->GetNumParameters() << L" parameters, but ";
			errormsg << CompileTimeParameters.top().size() << L" were provided";
			Throw(ParameterException(narrow(errormsg.str())));
		}

		StatementTypes.push(fs->GetReturnType());

		if(indirectinvoke)
			PendingEmitters.top().InvokeIndirect(statementnamehandle);
		else
			PendingEmitters.top().Invoke(statementnamehandle);

		CompileTimeParameters.pop();
		if(!CompileTimeParameters.empty())
		{
			CompileTimeParameter ctparam(L"@@expression", VM::EpochType_Expression);
			ctparam.ExpressionType = fs->GetReturnType();
			ctparam.ExpressionContents = PendingEmissionBuffers.top();
			CompileTimeParameters.top().push_back(ctparam);
		}
		else
			EmitterStack.top()->EmitBuffer(PendingEmissionBuffers.top());

		PendingEmitters.pop();
		PendingEmissionBuffers.pop();
	}
}

//
// Clean up parsing when a statement has been completely parsed
//
// This is invoked primarily when the outermost element of a statement has been located,
// meaning that no further code will be parsed without opening up a new statement.
//
// The primary responsibility of this function is to ensure that if a non-void function
// call or other statement was performed, the return value is popped back off the stack
// prior to moving on to the next statement. Otherwise, the stack would leak.
//
void CompilationSemantics::FinalizeStatement()
{
	if(!IsPrepass)
	{
		if(!StatementTypes.empty())
		{
			if(StatementTypes.top() != VM::EpochType_Void)
				EmitterStack.top()->PopStack(StatementTypes.top());
		}
		StatementTypes.c.clear();
	}

	PushedItemTypes.pop();
	TemporaryMemberList.clear();
}


//-------------------------------------------------------------------------------
// Assignment operations
//-------------------------------------------------------------------------------

//
// Begin parsing an assignment operation
//
// Note that by this point we already have parsed the left hand side of the assignment expression,
// so all we need to do is record that we're invoking the assignment operator, and prepare for
// parsing the right hand side of the expression.
//
void CompilationSemantics::BeginAssignment()
{
	BeginOpAssignment(L"=");
}

//
// Begin parsing an assignment, which may have additional side effects
//
// Note that for simplicity we invoke this function from BeginAssignment for regular assignments
// with no additional side effects. This is handled explicitly by not setting up compile-time
// parameter metadata for plain old assignments.
//
void CompilationSemantics::BeginOpAssignment(const std::wstring &identifier)
{
	if(!IsPrepass)
	{
		if(!LexicalScopeDescriptions[LexicalScopeStack.top()].HasVariable(TemporaryString))
			Throw(RecoverableException("The variable \"" + narrow(TemporaryString) + "\" is not defined in this scope"));
	}

	StatementNames.push(identifier);
	StatementParamCount.push(0);

	AssignmentTargets.push(AssignmentTarget(Session.StringPool.Pool(TemporaryString)));
	AssignmentTargets.top().Members.swap(TemporaryMemberList);
	TemporaryString.clear();

	if(!IsPrepass)
	{
		if(identifier == L"=")
			CompileTimeParameters.push(CompileTimeParameterVector());
		else
			CompileTimeParameters.push(CompileTimeParameterVector(1, CompileTimeParameter(L"lhs", GetEffectiveType(AssignmentTargets.top()))));

		InfixOperators.push(StringHandles());
		UnaryOperators.push(UnaryOperatorVector());

		PendingEmissionBuffers.push(ByteBuffer());
		PendingEmitters.push(ByteCodeEmitter(PendingEmissionBuffers.top()));
	}
}

//
// Finish parsing an assignment operation
//
// This function is invoked once both the left and right sides of the expression have been parsed,
// meaning that we can safely emit the assignment operator invocation code, and clean up.
//
void CompilationSemantics::CompleteAssignment()
{
	if(!IsPrepass)
	{
		VM::EpochTypeID expressiontype = GetEffectiveType(CompileTimeParameters.top().back());

		if(StatementNames.top() != L"=")
			AssignmentTargets.top().EmitCurrentValue(*EmitterStack.top(), LexicalScopeDescriptions.find(LexicalScopeStack.top())->second, Structures, StructureNames, Session.StringPool);

		EmitInfixOperand(*EmitterStack.top(), CompileTimeParameters.top().back());

		if(StatementNames.top() != L"=")
		{
			std::wstring assignmentop = StatementNames.top();
			StringHandle assignmentophandle = Session.StringPool.Pool(assignmentop);
			RemapFunctionToOverload(CompileTimeParameters.top(), 0, 2, 2, TypeVector(1, expressiontype), assignmentop, assignmentophandle);

			EmitterStack.top()->Invoke(assignmentophandle);
		}

		PendingEmitters.pop();
		PendingEmissionBuffers.pop();
		InfixOperators.pop();
		UnaryOperators.pop();

		AssignmentTargets.top().EmitReferenceBindings(*EmitterStack.top());
		EmitterStack.top()->AssignVariable();
		CompileTimeParameters.pop();
		StatementTypes.c.clear();

		// Check for chained assignments
		if(AssignmentTargets.size() > 1)
		{
			CompileTimeParameter ctparam(L"@@chainedassignment", VM::EpochType_Expression);
			ByteBuffer buffer;
			ByteCodeEmitter emitter(buffer);
			AssignmentTargets.top().EmitReferenceBindings(emitter);
			emitter.ReadReferenceOntoStack();
			ctparam.ExpressionContents.swap(buffer);
			CompileTimeParameters.top().push_back(ctparam);
		}

		if(expressiontype != GetEffectiveType(AssignmentTargets.top()))
		{
			StatementNames.pop();
			AssignmentTargets.pop();
			StatementParamCount.pop();
			Throw(TypeMismatchException("Right hand side of assignment does not match variable type"));
		}
	}
	StatementNames.pop();
	AssignmentTargets.pop();
	StatementParamCount.pop();
}

//
// Register that an assignment is accessing a structure member rather than a regular variable
//
void CompilationSemantics::RegisterAssignmentMember(const std::wstring& identifier)
{
	TemporaryMemberList.push_back(Session.StringPool.Pool(identifier));
}

//-------------------------------------------------------------------------------
// Entity invocation
//-------------------------------------------------------------------------------

//
// Signal the beginning of a set of parameters passed to an entity invocation
//
void CompilationSemantics::BeginEntityParams()
{
	// This has the same responsibilities as invoking a function at this point,
	// so we can safely delegate and avoid a lot of code duplication.
	BeginStatementParams();
}

//
// Signal the end of a set of parameters for an entity invocation
//
void CompilationSemantics::CompleteEntityParams(bool ispostfixcloser)
{
	std::wstring entityname = StatementNames.top();
	StringHandle entitynamehandle = Session.StringPool.Pool(entityname);

	StatementNames.pop();
	StatementParamCount.pop();

	StringHandle anonymousnamehandle = 0;
	if(!ispostfixcloser)
	{
		std::wstring anonymousname = AllocateAnonymousScopeName();
		anonymousnamehandle = Session.StringPool.Pool(anonymousname);
		CurrentEntities.push(anonymousname);

		LexicalScopeDescriptions.insert(std::make_pair(anonymousnamehandle, ScopeDescription(&LexicalScopeDescriptions.find(LexicalScopeStack.top())->second)));
		LexicalScopeStack.push(anonymousnamehandle);
	}

	if(!IsPrepass)
	{
		InfixOperators.pop();
		UnaryOperators.pop();

		bool valid = true;
		const CompileTimeParameterVector& entityparams = Session.GetCustomEntityByTag(EntityTypeTags.top()).Parameters;

		if(CompileTimeParameters.top().size() == entityparams.size())
		{
			for(size_t i = 0; i < CompileTimeParameters.top().size(); ++i)
			{
				if(CompileTimeParameters.top()[i].Type == VM::EpochType_Identifier)
				{
					if(GetLexicalScopeDescription(LexicalScopeStack.top()).GetVariableTypeByID(CompileTimeParameters.top()[i].Payload.StringHandleValue) != entityparams[i].Type)
					{
						valid = false;
						break;
					}
				}
				else if(CompileTimeParameters.top()[i].Type == VM::EpochType_Expression)
				{
					if(CompileTimeParameters.top()[i].ExpressionType != entityparams[i].Type)
					{
						valid = false;
						break;
					}
				}
				else if(CompileTimeParameters.top()[i].Type != entityparams[i].Type)
				{
					valid = false;
					break;
				}
			}
		}
		else
			valid = false;

		if(valid)
		{
			for(CompileTimeParameterVector::const_iterator iter = CompileTimeParameters.top().begin(); iter != CompileTimeParameters.top().end(); ++iter)
				EmitInfixOperand(*EmitterStack.top(), *iter);

			if(!ispostfixcloser)
				EmitterStack.top()->EnterEntity(EntityTypeTags.top(), anonymousnamehandle);
		}
		
		CompileTimeParameters.pop();

		PendingEmitters.pop();
		PendingEmissionBuffers.pop();

		if(!valid)
			Throw(ParameterException("Incorrect parameters to " + narrow(entityname)));
	}
}

//
// Signal the beginning of an entity chain
//
// Entity chains are used for implementing entity logic which needs to be executed
// multiple times, or needs to pass on through one of multiple potential handlers.
// This includes loops, if/elseif/else blocks, and switches.
//
void CompilationSemantics::BeginEntityChain()
{
	if(!IsPrepass)
		EmitterStack.top()->BeginChain();
}

//
// Signal the end of an entity chain
//
void CompilationSemantics::EndEntityChain()
{
	if(!IsPrepass)
		EmitterStack.top()->EndChain();
}


//-------------------------------------------------------------------------------
// Parameter validation
//-------------------------------------------------------------------------------

//
// Queue a parameter passed to a statement for later validation, incrementing the passed parameter count as we go
//
void CompilationSemantics::PushStatementParam()
{
	PushParam(L"@@passedparam");

	if(!IsPrepass)
	{
		CollapseUnaryOperators();
		InfixOperators.top().clear();
		UnaryOperators.top().clear();
		PendingEmissionBuffers.top().clear();
	}
}

//
// Queue a parameter that is part of an infix expression for later validation, incrementing the passed parameter count as we go
//
void CompilationSemantics::PushInfixParam()
{
	PushParam(L"@@infixoperand");
}

//
// Actually enqueue a parameter for future validation
//
void CompilationSemantics::PushParam(const std::wstring& paramname)
{
	// Set up the compile-time parameter payload
	switch(PushedItemTypes.top())
	{
	case ITEMTYPE_STATEMENT:
		// Nothing to do, the nested statement handler has already taken care of things
		break;

	case ITEMTYPE_STRING:
		{
			CompileTimeParameter ctparam(paramname, VM::EpochType_Identifier);
			StringHandle handle = Session.StringPool.Pool(Strings.top());
			ctparam.StringPayload = Strings.top();
			ctparam.Payload.StringHandleValue = handle;
			Strings.pop();
			if(!IsPrepass)
				CompileTimeParameters.top().push_back(ctparam);
		}
		break;

	case ITEMTYPE_STRINGLITERAL:
		{
			CompileTimeParameter ctparam(paramname, VM::EpochType_String);
			ctparam.Payload.StringHandleValue = StringLiterals.top();
			StringLiterals.pop();
			if(!IsPrepass)
				CompileTimeParameters.top().push_back(ctparam);
		}
		break;

	case ITEMTYPE_INTEGERLITERAL:
		{
			CompileTimeParameter ctparam(paramname, VM::EpochType_Integer);
			ctparam.Payload.IntegerValue = IntegerLiterals.top();
			IntegerLiterals.pop();
			if(!IsPrepass)
				CompileTimeParameters.top().push_back(ctparam);
		}
		break;

	case ITEMTYPE_BOOLEANLITERAL:
		{
			CompileTimeParameter ctparam(paramname, VM::EpochType_Boolean);
			ctparam.Payload.BooleanValue = BooleanLiterals.top();
			BooleanLiterals.pop();
			if(!IsPrepass)
				CompileTimeParameters.top().push_back(ctparam);
		}
		break;

	case ITEMTYPE_REALLITERAL:
		{
			CompileTimeParameter ctparam(paramname, VM::EpochType_Real);
			ctparam.Payload.RealValue = RealLiterals.top();
			RealLiterals.pop();
			if(!IsPrepass)
				CompileTimeParameters.top().push_back(ctparam);
		}
		break;

	default:
		throw NotImplementedException("The parser stack contains something we aren't ready to handle in CompilationSemantics::PushParam");
	}

	PushedItemTypes.pop();
}


//-------------------------------------------------------------------------------
// Finalization of parsing process
//-------------------------------------------------------------------------------

//
// Signal the end of the parsed source code file, and emit the final metadata instructions for the module
//
void CompilationSemantics::Finalize()
{
	if(!IsPrepass)
	{
		// Generate any pattern-matching resolver functions needed
		for(FunctionSignatureMap::const_iterator iter = NeededPatternResolvers.begin(); iter != NeededPatternResolvers.end(); ++iter)
		{
			EmitterStack.top()->DefineLexicalScope(iter->first, 0, iter->second.GetNumParameters());
			for(size_t i = 0; i < iter->second.GetNumParameters(); ++i)
				EmitterStack.top()->LexicalScopeEntry(Session.StringPool.Pool(iter->second.GetParameter(i).Name), iter->second.GetParameter(i).Type, iter->second.GetParameter(i).IsReference, VARIABLE_ORIGIN_PARAMETER);

			EmitterStack.top()->EnterPatternResolver(iter->first);
			std::pair<AllOverloadsMap::const_iterator, AllOverloadsMap::const_iterator> range = OriginalFunctionsForPatternResolution.equal_range(iter->first);
			for(AllOverloadsMap::const_iterator originalfunciter = range.first; originalfunciter != range.second; ++originalfunciter)
				EmitterStack.top()->ResolvePattern(originalfunciter->second, Session.FunctionSignatures.find(originalfunciter->second)->second);
			EmitterStack.top()->ExitPatternResolver();
		}

		// Pool string literals and identifiers
		for(std::map<StringHandle, std::wstring>::const_reverse_iterator iter = Session.StringPool.GetInternalPool().rbegin(); iter != Session.StringPool.GetInternalPool().rend(); ++iter)
			EmitterStack.top()->PoolString(iter->first, iter->second);

		// Generate lexical scope metadata
		for(ScopeMap::const_iterator iter = LexicalScopeDescriptions.begin(); iter != LexicalScopeDescriptions.end(); ++iter)
		{
			EmitterStack.top()->DefineLexicalScope(iter->first, FindLexicalScopeName(iter->second.ParentScope), iter->second.GetVariableCount());
			for(size_t i = 0; i < iter->second.GetVariableCount(); ++i)
				EmitterStack.top()->LexicalScopeEntry(Session.StringPool.Pool(iter->second.GetVariableName(i)), iter->second.GetVariableTypeByIndex(i), iter->second.IsReference(i), iter->second.GetVariableOrigin(i));
		}

		// Generate structure definition metadata
		for(StructureDefinitionMap::const_iterator iter = Structures.begin(); iter != Structures.end(); ++iter)
		{
			EmitterStack.top()->DefineStructure(iter->first, iter->second.GetNumMembers());
			for(size_t i = 0; i < iter->second.GetNumMembers(); ++i)
				EmitterStack.top()->StructureMember(iter->second.GetMemberName(i), iter->second.GetMemberType(i));
		}
	}
}


//-------------------------------------------------------------------------------
// Lexical scope metadata
//-------------------------------------------------------------------------------

//
// Track the creation of a lexical scope; used to emit metadata for the scope's contents later on
//
void CompilationSemantics::AddLexicalScope(StringHandle scopename)
{
	if(LexicalScopeDescriptions.find(scopename) != LexicalScopeDescriptions.end())
		Throw(RecoverableException("An entity with the identifier \"" + narrow(Session.StringPool.GetPooledString(scopename)) + "\" has already been defined"));
	
	LexicalScopeDescriptions.insert(std::make_pair(scopename, ScopeDescription()));
	LexicalScopeStack.push(scopename);
}

//
// Retrieve the description of the given lexical scope
//
ScopeDescription& CompilationSemantics::GetLexicalScopeDescription(StringHandle scopename)
{
	ScopeMap::iterator iter = LexicalScopeDescriptions.find(scopename);
	if(iter == LexicalScopeDescriptions.end())
		throw InvalidIdentifierException("No lexical scope has been attached to this identifier");

	return iter->second;
}


//-------------------------------------------------------------------------------
// Function overload management
//-------------------------------------------------------------------------------

//
// Allocate an internal alias for an overloaded function name
//
// When a function is overloaded, the overloads are given magic name suffixes to help distinguish
// them from other function overloads. This function creates and pools a new alias if needed; for
// non-overload situations it just returns the given original name handle.
//
StringHandle CompilationSemantics::AllocateNewOverloadedFunctionName(StringHandle originalname)
{
	StringHandleSet& overloadednames = Session.FunctionOverloadNames[originalname];
	if(overloadednames.empty())
	{
		overloadednames.insert(originalname);
		return originalname;
	}

	std::wostringstream mangled;
	mangled << Session.StringPool.GetPooledString(originalname) << L"@@overload@@" << overloadednames.size();
	StringHandle ret = Session.StringPool.Pool(mangled.str());
	overloadednames.insert(ret);
	return ret;
}

//
// Given a set of parameters, look up the appropriate matching function overload
//
void CompilationSemantics::RemapFunctionToOverload(const CompileTimeParameterVector& params, size_t paramoffset, size_t knownparams, size_t paramlimit, const TypeVector& possiblereturntypes, std::wstring& out_remappedname, StringHandle& out_remappednamehandle) const
{
	StringVector matchingnames;
	StringHandles matchingnamehandles;

	GetAllMatchingOverloads(params, paramoffset, knownparams, paramlimit, possiblereturntypes, out_remappedname, out_remappednamehandle, matchingnames, matchingnamehandles);

	if(!matchingnamehandles.empty())
	{
		if(matchingnamehandles.size() > 1)
			Throw(RecoverableException("Multiple overloads matched for \"" + narrow(out_remappedname) + "\" - not sure which one to call"));

		out_remappednamehandle = matchingnamehandles.front();
		out_remappedname = Session.StringPool.GetPooledString(out_remappednamehandle);
		return;
	}

	Throw(RecoverableException("No function overload for \"" + narrow(out_remappedname) + "\" takes a matching parameter set"));
}

//
// Retrieve a list of all overloads which match the given criteria
//
void CompilationSemantics::GetAllMatchingOverloads(const CompileTimeParameterVector& params, size_t paramoffset, size_t knownparams, size_t paramlimit, const TypeVector& possiblereturntypes, const std::wstring& originalname, StringHandle originalnamehandle, StringVector& out_names, StringHandles& out_namehandles) const
{
	OverloadMap::const_iterator overloadsiter = Session.FunctionOverloadNames.find(originalnamehandle);
	if(overloadsiter == Session.FunctionOverloadNames.end())
	{
		out_names.push_back(originalname);
		out_namehandles.push_back(originalnamehandle);
		return;
	}

	bool patternmatching = false;
	bool differingreturntypes = false;
	bool remaptopatternresolver = false;

	const StringHandleSet& overloadednames = overloadsiter->second;
	for(StringHandleSet::const_iterator iter = overloadednames.begin(); iter != overloadednames.end(); ++iter)
	{
		FunctionSignatureSet::const_iterator signatureiter = Session.FunctionSignatures.find(*iter);
		if(signatureiter == Session.FunctionSignatures.end())
			throw FatalException("Tried to map a function overload to an undefined function signature");

		if(!possiblereturntypes.empty())
		{
			bool diff = true;
			for(TypeVector::const_iterator returntypeiter = possiblereturntypes.begin(); returntypeiter != possiblereturntypes.end(); ++returntypeiter)
			{
				if(signatureiter->second.GetReturnType() == *returntypeiter)
				{
					diff = false;
					break;
				}
			}

			if(diff)
				differingreturntypes = true;
		}
	}

	StringHandleSet matches;

	for(StringHandleSet::const_iterator iter = overloadednames.begin(); iter != overloadednames.end(); ++iter)
	{
		FunctionSignatureSet::const_iterator signatureiter = Session.FunctionSignatures.find(*iter);

		size_t numparams = std::min(params.size() - paramoffset, paramlimit);
		if(numparams > signatureiter->second.GetNumParameters())
			continue;

		if(knownparams < std::numeric_limits<size_t>::max() && signatureiter->second.GetNumParameters() != knownparams)
			continue;

		if(!possiblereturntypes.empty())
		{
			bool matchesreturntype = false;
			for(TypeVector::const_iterator returntypeiter = possiblereturntypes.begin(); returntypeiter != possiblereturntypes.end(); ++returntypeiter)
			{
				if(signatureiter->second.GetReturnType() == *returntypeiter)
				{
					matchesreturntype = true;
					break;
				}
			}

			if(!matchesreturntype)
				continue;
		}

		bool matched = true;
		bool patternsucceeded = true;
		bool patternsucceededonvalue = false;
		for(size_t i = paramoffset; i < std::min(params.size(), paramoffset + paramlimit); ++i)
		{
			if(signatureiter->second.GetParameter(i - paramoffset).Name == L"@@patternmatched")
			{
				patternmatching = true;
				switch(params[i].Type)
				{
				case VM::EpochType_Integer:
					if(signatureiter->second.GetParameter(i - paramoffset).Payload.IntegerValue != params[i].Payload.IntegerValue)
						patternsucceeded = false;
					else
						patternsucceededonvalue = true;
					break;

				// If an identifier is passed, we have to check if the overload wants an identifier or a variable
				// If a variable is preferred, we always delegate to the runtime match default
				case VM::EpochType_Identifier:
					if(signatureiter->second.GetParameter(i - paramoffset).Type == VM::EpochType_Identifier)
					{
						if(signatureiter->second.GetParameter(i - paramoffset).Payload.StringHandleValue != params[i].Payload.StringHandleValue)
							patternsucceeded = false;
						else
							patternsucceededonvalue = true;
					}
					else
						patternsucceeded = false;
					break;

				// Always delegate to the runtime match default if an expression is passed
				case VM::EpochType_Expression:
					patternsucceeded = false;
					break;

				default:
					throw NotImplementedException("Unsupported pattern-matched parameter type");
				}

				if(!matched || !patternsucceeded)
					break;
			}
			else
			{
				if(params[i].Type == VM::EpochType_Identifier)
				{
					if(signatureiter->second.GetParameter(i - paramoffset).Type != VM::EpochType_Identifier)
					{
						ScopeMap::const_iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
						if(iter == LexicalScopeDescriptions.end())
							throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");

						if(signatureiter->second.GetParameter(i - paramoffset).Type == VM::EpochType_Function)
						{
							FunctionSignatureSet::const_iterator funciter = Session.FunctionSignatures.find(params[i].Payload.StringHandleValue);
							if(funciter == Session.FunctionSignatures.end())
								Throw(RecoverableException("No function by the name \"" + narrow(params[i].StringPayload) + "\" was found in this scope"));

							if(!funciter->second.Matches(signatureiter->second.GetFunctionSignature(i - paramoffset)))
							{
								patternsucceeded = false;
								matched = false;
								break;
							}
						}
						else
						{
							if(!iter->second.HasVariable(params[i].StringPayload))
							{
								patternsucceeded = false;
								matched = false;
								break;
							}

							if(iter->second.GetVariableTypeByID(params[i].Payload.StringHandleValue) != signatureiter->second.GetParameter(i - paramoffset).Type)
							{
								patternsucceeded = false;
								matched = false;
								break;
							}
						}
					}
				}
				else if(params[i].Type == VM::EpochType_Expression)
				{
					if(signatureiter->second.GetParameter(i - paramoffset).Type != params[i].ExpressionType)
					{
						patternsucceeded = false;
						matched = false;
						break;
					}
				}
				else if(params[i].Type != signatureiter->second.GetParameter(i - paramoffset).Type)
				{
					patternsucceeded = false;
					matched = false;
					break;
				}
			}
		}

		if(matched)
		{
			if(patternmatching)
			{
				if(patternsucceeded)
				{
					if(patternsucceededonvalue)
					{
						remaptopatternresolver = false;
						matches.insert(*iter);
						break;
					}
				}
				else if(!differingreturntypes)
					remaptopatternresolver = true;
			}
			else
				matches.insert(*iter);
		}
	}

	if(remaptopatternresolver)
	{
		std::wstring resolvername = GetPatternMatchResolverName(originalname);
		StringHandle resolvernamehandle = Session.StringPool.Pool(resolvername);
		out_names.push_back(resolvername);
		out_namehandles.push_back(resolvernamehandle);
		return;
	}

	for(StringHandleSet::const_iterator iter = matches.begin(); iter != matches.end(); ++iter)
	{
		out_names.push_back(Session.StringPool.GetPooledString(*iter));
		out_namehandles.push_back(*iter);
	}
}


//-------------------------------------------------------------------------------
// Function tagging
//-------------------------------------------------------------------------------

//
// Note the beginning of a function tag
//
// Function tags are used to annotate functions and give them special properties or behaviors.
// For instance, a function can be tagged as "external" to link it to a function in a separate
// C-compatible DLL interface, or "pure" to prohibit the use of side effects in the code body.
//
void CompilationSemantics::BeginFunctionTag(const std::wstring& tagname)
{
	TemporaryString = tagname;
}

//
// End a function tag and process its parameters, if any
//
void CompilationSemantics::CompleteFunctionTag()
{
	FunctionTagHelperTable::const_iterator iter = Session.FunctionTagHelpers.find(TemporaryString);
	if(iter != Session.FunctionTagHelpers.end())
	{
		CompileTimeParameterVector ctparams;
		while(!PushedItemTypes.empty())
		{
			CompileTimeParameter ctparam(L"@@tagparam", VM::EpochType_Error);

			switch(PushedItemTypes.top())
			{
			case ITEMTYPE_STRINGLITERAL:
				ctparam.Type = VM::EpochType_String;
				ctparam.StringPayload = Session.StringPool.GetPooledString(StringLiterals.top());
				StringLiterals.pop();
				break;

			case ITEMTYPE_INTEGERLITERAL:
				ctparam.Type = VM::EpochType_Integer;
				ctparam.Payload.IntegerValue = IntegerLiterals.top();
				IntegerLiterals.pop();
				break;

			case ITEMTYPE_BOOLEANLITERAL:
				ctparam.Type = VM::EpochType_Boolean;
				ctparam.Payload.BooleanValue = BooleanLiterals.top();
				BooleanLiterals.pop();
				break;

			case ITEMTYPE_REALLITERAL:
				ctparam.Type = VM::EpochType_Real;
				ctparam.Payload.RealValue = RealLiterals.top();
				RealLiterals.pop();
				break;

			default:
				Throw(RecoverableException("Invalid parameter to function tag"));
			}

			ctparams.insert(ctparams.begin(), ctparam);
			PushedItemTypes.pop();
		}

		TagHelperReturn ret = iter->second(LexicalScopeStack.top(), ctparams, IsPrepass);
		if(!ret.InvokeRuntimeFunction.empty())
			PendingEmitters.top().Invoke(Session.StringPool.Pool(ret.InvokeRuntimeFunction));

		if(!ret.MetaTag.empty())
			EmitterStack.top()->TagData(LexicalScopeStack.top(), ret.MetaTag, ret.MetaTagData);

		if(ret.LinkToCompileTimeHelper)
			CompileTimeHelpers[LexicalScopeStack.top()] = ret.LinkToCompileTimeHelper;

		if(ret.SetConstructorFunction)
			ConstructorNames.insert(ret.SetConstructorFunction);
	}
	else
	{
		EntityTypeTags.pop();
		CurrentEntities.pop();
		LexicalScopeStack.pop();
		CleanAllPushedItems();
		Fail();
		Throw(RecoverableException("Invalid function tag"));
	}
}


//-------------------------------------------------------------------------------
// Higher order functions
//-------------------------------------------------------------------------------

//
// Store the name of a higher order function parameter
//
// This must simply hold the name in a temporary string as we have not yet finished recognizing the
// higher-order function declaration in the parser.
//
void CompilationSemantics::StoreHigherOrderFunctionName(const std::wstring& functionname)
{
	TemporaryString = functionname;
}

//
// Register the beginning of the parameter list for a higher-order function
//
void CompilationSemantics::BeginHigherOrderFunctionParams()
{
	HigherOrderFunctionSignatures.push(FunctionSignature());
}

//
// Register the end of the parameter list for a higher-order function
//
void CompilationSemantics::EndHigherOrderFunctionParams()
{
	// Nothing to do at this point
}

//
// Register a parameter in the parameter list of a higher-order function
//
void CompilationSemantics::RegisterHigherOrderFunctionParam(const std::wstring& nameoftype)
{
	HigherOrderFunctionSignatures.top().AddParameter(L"@@higherorderparam", LookupTypeName(nameoftype), false);
}

//
// Register the beginning of the return list for a higher-order function
//
void CompilationSemantics::BeginHigherOrderFunctionReturns()
{
	// Aaaand still nothing to do
}

//
// Register the end of the return list for a higher-order function
//
void CompilationSemantics::EndHigherOrderFunctionReturns()
{
	FunctionSignatureStack.top().AddParameter(TemporaryString, VM::EpochType_Function, false);

	if(!IsPrepass)
	{
		ScopeMap::iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
		if(iter == LexicalScopeDescriptions.end())
			throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");
		
		iter->second.AddVariable(TemporaryString, Session.StringPool.Pool(TemporaryString), VM::EpochType_Function, false, VARIABLE_ORIGIN_PARAMETER);
	}
	else
		FunctionSignatureStack.top().SetFunctionSignature(FunctionSignatureStack.top().GetNumParameters() - 1, HigherOrderFunctionSignatures.top());

	HigherOrderFunctionSignatures.pop();
}

//
// Register a return value in the return list of a higher-order function
//
void CompilationSemantics::RegisterHigherOrderFunctionReturn(const std::wstring& nameoftype)
{
	HigherOrderFunctionSignatures.top().SetReturnType(LookupTypeName(nameoftype));
}



//-------------------------------------------------------------------------------
// Structures
//-------------------------------------------------------------------------------

namespace
{
	void CompileConstructorStructure(const std::wstring& functionname, SemanticActionInterface& semantics, ScopeDescription& scope, const CompileTimeParameterVector& compiletimeparams)
	{
		VariableOrigin origin = (semantics.IsInReturnDeclaration() ? VARIABLE_ORIGIN_RETURN : VARIABLE_ORIGIN_LOCAL);
		scope.AddVariable(compiletimeparams[0].StringPayload, compiletimeparams[0].Payload.StringHandleValue, semantics.LookupTypeName(functionname), false, origin);
	}
}

//
// Save off the name of a structure so it can be used later to register the type
//
void CompilationSemantics::StoreStructureName(const std::wstring& identifier)
{
	StructureName = identifier;
}

//
// Save off the name of a structure member's type so we can register the member later
//
void CompilationSemantics::StoreStructureMemberType(const std::wstring& type)
{
	if(IsPrepass)
		StructureMemberType = LookupTypeName(type);
}

//
// Register a structure member
//
void CompilationSemantics::RegisterStructureMember(const std::wstring& identifier)
{
	if(IsPrepass)
		StructureMembers.push_back(StructureMemberTypeNamePair(StructureMemberType, Session.StringPool.Pool(identifier)));
}

//
// Register that a structure member is a function reference
//
void CompilationSemantics::RegisterStructureMemberIsFunction()
{
	if(IsPrepass)
		HigherOrderFunctionSignatures.push(FunctionSignature());
}

//
// Register a parameter passed to a function reference stored in a structure member
//
void CompilationSemantics::RegisterStructureFunctionRefParam(const std::wstring& paramtypename)
{
	if(IsPrepass)
		HigherOrderFunctionSignatures.top().AddParameter(L"@@param", LookupTypeName(paramtypename), false);
}

//
// Register the return type of a function reference stored in a structure member
//
void CompilationSemantics::RegisterStructureFunctionRefReturn(const std::wstring& returntypename)
{
	if(IsPrepass)
	{
		HigherOrderFunctionSignatures.top().SetReturnType(LookupTypeName(returntypename));

		// TODO - validate that the structure member name is unique
		// TODO - support void functions in structure members

		StructureMembers.push_back(StructureMemberTypeNamePair(VM::EpochType_Function, Session.StringPool.Pool(TemporaryString)));
		StructureFunctionSignatures[TemporaryString] = HigherOrderFunctionSignatures.top();
		TemporaryString.clear();

		HigherOrderFunctionSignatures.pop();
	}
}

//
// Assemble the parsed structure members into a final type definition and register it
//
const std::wstring& CompilationSemantics::CreateStructureType()
{
	if(!IsPrepass)
		throw FatalException("Semantic action triggered in compilation phase which should only occur during prepass phase!");

	StringHandle idhandle = Session.StringPool.Pool(StructureName);
	VM::EpochTypeID type = ++CustomTypeIDCounter;

	Structures[type] = StructureDefinition();
	StructureNames[idhandle] = type;

	StringHandle varconstructorname = AllocateNewOverloadedFunctionName(idhandle);
	StringHandle anonconstructorname = AllocateNewOverloadedFunctionName(idhandle);

	// Now register a constructor function for the type
	CompileTimeHelpers[varconstructorname] = CompileConstructorStructure;

	GenerateConstructor(varconstructorname, type, true);
	GenerateConstructor(anonconstructorname, type, false);


	// Create member accessors
	for(StructureMemberList::const_iterator iter = StructureMembers.begin(); iter != StructureMembers.end(); ++iter)
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", type, true);
		signature.AddPatternMatchedParameterIdentifier(iter->second);
		signature.SetReturnType(iter->first);
		StringHandle overloadidhandle = Session.StringPool.Pool(L".@@" + StructureName + L"@@" + Session.StringPool.GetPooledString(iter->second));
		AddToMapNoDupe(Session.FunctionSignatures, std::make_pair(overloadidhandle, signature));
		Session.FunctionOverloadNames[Session.StringPool.Pool(L".")].insert(overloadidhandle);

		EmitterStack.top()->EnterFunction(overloadidhandle);
		EmitterStack.top()->CopyFromStructure(Session.StringPool.Pool(L"identifier"), Session.StringPool.Pool(L"member"));
		EmitterStack.top()->ExitFunction();

		LexicalScopeDescriptions[overloadidhandle].AddVariable(L"identifier", Session.StringPool.Pool(L"identifier"), type, false, VARIABLE_ORIGIN_PARAMETER);
		LexicalScopeDescriptions[overloadidhandle].AddVariable(L"member", Session.StringPool.Pool(L"member"), VM::EpochType_Identifier, false, VARIABLE_ORIGIN_PARAMETER);
		LexicalScopeDescriptions[overloadidhandle].AddVariable(L"ret", Session.StringPool.Pool(L"ret"), iter->first, false, VARIABLE_ORIGIN_RETURN);
	}

	StructureMembers.clear();
	StructureFunctionSignatures.clear();			// TODO - add support for signature validation of function references in structure members
	return Session.StringPool.GetPooledString(idhandle);
}

//
// Generate a constructor (possibly anonymous) for a structure type
//
void CompilationSemantics::GenerateConstructor(StringHandle constructorname, VM::EpochTypeID type, bool takesidentifier)
{
	FunctionSignature signature;
	if(takesidentifier)
	{
		signature.AddParameter(L"identifier", VM::EpochType_Identifier, true);
		LexicalScopeDescriptions[constructorname].AddVariable(L"identifier", Session.StringPool.Pool(L"identifier"), VM::EpochType_Identifier, true, VARIABLE_ORIGIN_PARAMETER);
	}

	// Add parameters for each structure member
	size_t index = takesidentifier ? 1 : 0;		// Account for the fact that the constructed variable's identifier occupies slot 0
	for(StructureMemberList::const_iterator iter = StructureMembers.begin(); iter != StructureMembers.end(); ++iter)
	{
		const std::wstring& identifier = Session.StringPool.GetPooledString(iter->second);
		signature.AddParameter(identifier, iter->first, false);
		if(iter->first == VM::EpochType_Function)
			signature.SetFunctionSignature(index, StructureFunctionSignatures.find(identifier)->second);
		const StructureDefinition* structdefinition = NULL;
		if(iter->first > VM::EpochType_CustomBase)
			structdefinition = &Structures[iter->first];
		Structures[type].AddMember(iter->second, iter->first, structdefinition);
		LexicalScopeDescriptions[constructorname].AddVariable(identifier, iter->second, iter->first, false, VARIABLE_ORIGIN_PARAMETER);
		++index;
	}

	if(!takesidentifier)
		signature.SetReturnType(type);

	AddToMapNoDupe(Session.FunctionSignatures, std::make_pair(constructorname, signature));

	// Create constructor
	EmitterStack.top()->EnterFunction(constructorname);
	EmitterStack.top()->AllocateStructure(type);

	StringHandle targetvariable = 0;
	if(takesidentifier)
		targetvariable = Session.StringPool.Pool(L"identifier");
	else
	{
		targetvariable = Session.StringPool.Pool(L"@@anonymous");
		LexicalScopeDescriptions[constructorname].AddVariable(L"@@anonymous", targetvariable, type, false, VARIABLE_ORIGIN_LOCAL);

		StringHandle rethandle = Session.StringPool.Pool(L"ret");
		LexicalScopeDescriptions[constructorname].AddVariable(L"ret", rethandle, type, false, VARIABLE_ORIGIN_RETURN);
	}

	EmitterStack.top()->BindReference(targetvariable);
	EmitterStack.top()->AssignVariable();

	for(StructureMemberList::const_iterator iter = StructureMembers.begin(); iter != StructureMembers.end(); ++iter)
	{
		EmitterStack.top()->PushVariableValue(iter->second, iter->first);
		EmitterStack.top()->AssignStructure(targetvariable, iter->second);
	}

	if(!takesidentifier)
		EmitterStack.top()->SetReturnRegister(targetvariable);

	EmitterStack.top()->ExitFunction();
}


//-------------------------------------------------------------------------------
// Additional helpers
//-------------------------------------------------------------------------------

//
// Emit the contents of the most recent pending emitter into the current emitter
//
// Pending emitters are used to buffer emitted code for injection into another buffer. For example,
// when generating prolog code for an entity, it is necessary to buffer that prolog code during the
// compilation process so that it can be injected inside the entity's code block.
//
void CompilationSemantics::EmitPendingCode()
{
	if(!IsPrepass)
	{
		if(!PendingEmitters.empty())
		{
			EmitterStack.top()->EmitBuffer(PendingEmissionBuffers.top());
			PendingEmitters.pop();
			PendingEmissionBuffers.pop();
		}
	}
}

//
// Convert a type name into an internal type annotation constant
//
VM::EpochTypeID CompilationSemantics::LookupTypeName(const std::wstring& type) const
{
	if(type == L"integer")
		return VM::EpochType_Integer;
	else if(type == L"integer16")
		return VM::EpochType_Integer16;
	else if(type == L"string")
		return VM::EpochType_String;
	else if(type == L"boolean")
		return VM::EpochType_Boolean;
	else if(type == L"real")
		return VM::EpochType_Real;
	else if(type == L"buffer")
		return VM::EpochType_Buffer;
	else if(type == L"identifier")
		return VM::EpochType_Identifier;

	StringHandle handle = Session.GetOverloadRawName(Session.StringPool.Pool(type));
	StructureNameMap::const_iterator iter = StructureNames.find(handle);
	if(iter != StructureNames.end())
		return iter->second;

	throw NotImplementedException("Cannot map the type name \"" + narrow(type) + "\" to an internal type ID");
}

//
// Decorate a function name to produce the internal name of the function used to resolve pattern matches for the wrapped function
//
std::wstring CompilationSemantics::GetPatternMatchResolverName(const std::wstring& originalname) const
{
	return originalname + L"@@resolve_pattern_match";
}


//
// Traverse up the call chain of an expression, resolving overloads along the way
// as possible, looking for the expected type of the current expression term. This
// is used to allow function overloads to differ only by their return types, where
// the expected type is used to infer which overload should be invoked.
//
CompilationSemantics::TypeVector CompilationSemantics::WalkCallChainForExpectedTypes(size_t index) const
{
	if(StatementNames.empty() || StatementNames.c.size() <= index)
		return TypeVector();

	std::wstring name = StatementNames.c.at(index);

	if(name == L"=")
	{
		return TypeVector(1, GetEffectiveType(AssignmentTargets.top()));
	}
	else
	{
		StringHandle namehandle = Session.StringPool.Pool(name);
		unsigned paramindex = StatementParamCount.c.at(index);

		TypeVector outerexpectedtypes;
		if(index > 0)
			outerexpectedtypes = WalkCallChainForExpectedTypes(index - 1);

		StringVector outnames;
		StringHandles outnamehandles;
		GetAllMatchingOverloads(CompileTimeParameters.c.at(index), paramindex, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), outerexpectedtypes, name, namehandle, outnames, outnamehandles);

		TypeVector ret;

		for(size_t i = 0; i < outnames.size(); ++i)
		{
			FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(outnamehandles[i]);
			if(iter != Session.FunctionSignatures.end())
				ret.push_back(iter->second.GetParameter(paramindex).Type);
			else
			{
				try
				{
					const EntityDescription& entitydescription = Session.GetCustomEntityByName(outnamehandles[i]);
					if(entitydescription.Parameters.size() > paramindex)
						ret.push_back(entitydescription.Parameters[paramindex].Type);
				}
				catch(InvalidIdentifierException&)
				{
					// Must not have been a valid entity! Just keep going.
				}
			}
		}

		if(ret.empty())
			Throw(RecoverableException("The function \"" + narrow(name) + "\" is not defined in this scope"));

		return ret;
	}
}


//
// Retrieve a numerical score indicating the operator precedence level of a given operator
//
int CompilationSemantics::GetOperatorPrecedence(StringHandle operatorname) const
{
	for(PrecedenceTable::const_iterator iter = Session.OperatorPrecedences.begin(); iter != Session.OperatorPrecedences.end(); ++iter)
	{
		if(iter->second == operatorname)
			return iter->first;
	}

	Throw(RecoverableException("Infix operator has no precedence level defined"));
	return 0;		// Just to satisfy the compiler, which can't detect that Throw() will bail us with an exception
}


//
// Helper for emitting an infix expression operand
//
void CompilationSemantics::EmitInfixOperand(ByteCodeEmitter& emitter, const CompileTimeParameter& ctparam)
{
	switch(ctparam.Type)
	{
	case VM::EpochType_Integer:
		emitter.PushIntegerLiteral(ctparam.Payload.IntegerValue);
		break;

	case VM::EpochType_String:
		emitter.PushStringLiteral(ctparam.Payload.StringHandleValue);
		break;

	case VM::EpochType_Boolean:
		emitter.PushBooleanLiteral(ctparam.Payload.BooleanValue);
		break;

	case VM::EpochType_Real:
		emitter.PushRealLiteral(ctparam.Payload.RealValue);
		break;

	case VM::EpochType_Identifier:
		emitter.PushVariableValue(ctparam.Payload.StringHandleValue, LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.GetVariableTypeByID(ctparam.Payload.StringHandleValue));
		break;

	case VM::EpochType_Expression:
		emitter.EmitBuffer(ctparam.ExpressionContents);
		break;

	case VM::EpochType_Buffer:
		emitter.PushBufferHandle(ctparam.Payload.BufferHandleValue);
		break;

	default:
		throw NotImplementedException("Unsupported operand type in infix expression");
	}
}

//
// Generate an internal name for an anonymous lexical scope
//
std::wstring CompilationSemantics::AllocateAnonymousScopeName()
{
	std::wostringstream ret;
	ret << L"@@anonscope@@" << AnonymousScopeCounter;
	++AnonymousScopeCounter;
	return ret.str();
}

//
// Switch between prepass and final compilation pass modes
//
void CompilationSemantics::SetPrepassMode(bool isprepass)
{
	IsPrepass = isprepass;
	AnonymousScopeCounter = 0;
}

//
// Look up the name handle of the lexical scope description at the given address
//
StringHandle CompilationSemantics::FindLexicalScopeName(const ScopeDescription* scope) const
{
	for(ScopeMap::const_iterator iter = LexicalScopeDescriptions.begin(); iter != LexicalScopeDescriptions.end(); ++iter)
	{
		if(&iter->second == scope)
			return iter->first;
	}

	return 0;
}

//
// Look up the entity type tag of the entity with the given string name
//
Bytecode::EntityTag CompilationSemantics::LookupEntityTag(StringHandle identifier) const
{
	for(EntityTable::const_iterator iter = Session.CustomEntities.begin(); iter != Session.CustomEntities.end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = Session.ChainedEntities.begin(); iter != Session.ChainedEntities.end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = Session.PostfixEntities.begin(); iter != Session.PostfixEntities.end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	for(EntityTable::const_iterator iter = Session.PostfixClosers.begin(); iter != Session.PostfixClosers.end(); ++iter)
	{
		if(iter->second.StringName == identifier)
			return iter->first;
	}

	Throw(RecoverableException("Invalid entity"));

	// Just to satisfy compilers which can't tell that Throw() will bail us out
	return Bytecode::EntityTags::Invalid;
}

//
// Validate parameters to an infix operator, emitting error messages if applicable
//
void CompilationSemantics::VerifyInfixOperandTypes(StringHandle infixoperator, VM::EpochTypeID op1type, VM::EpochTypeID op2type)
{
	if(Session.FunctionSignatures.find(infixoperator)->second.GetParameter(0).Type != op1type)
	{
		std::wcout << L"Error in file \"" << StripPath(widen(ParsePosition.get_position().file)) << L"\" on line " << ParsePosition.get_position().line << L":\n";
		std::wcout << L"The left hand side of the operator " << Session.StringPool.GetPooledString(infixoperator) << " is of the wrong type" << std::endl << std::endl;

		Fail();
	}

	if(Session.FunctionSignatures.find(infixoperator)->second.GetParameter(1).Type != op2type)
	{
		std::wcout << L"Error in file \"" << StripPath(widen(ParsePosition.get_position().file)) << L"\" on line " << ParsePosition.get_position().line << L":\n";
		std::wcout << L"The right hand side of the operator " << Session.StringPool.GetPooledString(infixoperator) << " is of the wrong type" << std::endl << std::endl;

		Fail();
	}
}

//
// Determine the effective type of a literal, expression, or variable
//
VM::EpochTypeID CompilationSemantics::GetEffectiveType(const CompileTimeParameter& param) const
{
	if(param.Type == VM::EpochType_Identifier)
		return LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.GetVariableTypeByID(param.Payload.StringHandleValue);
	else if(param.Type == VM::EpochType_Expression)
		return param.ExpressionType;

	return param.Type;
}

//
// Determine the effective type of an assignment target
//
VM::EpochTypeID CompilationSemantics::GetEffectiveType(const AssignmentTarget& assignmenttarget) const
{
	VM::EpochTypeID vartype = LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.GetVariableTypeByID(assignmenttarget.Variable);
	if (assignmenttarget.Members.empty())
		return vartype;

	for(StringHandles::const_iterator iter = assignmenttarget.Members.begin(); iter != assignmenttarget.Members.end(); ++iter)
	{
		const StructureDefinition& structdef = Structures.find(vartype)->second;
		vartype = structdef.GetMemberType(structdef.FindMember(*iter));
	}
	return vartype;
}

//
// Clear the parser stacks
//
void CompilationSemantics::CleanAllPushedItems()
{
	while(!PushedItemTypes.empty())
	{
		switch(PushedItemTypes.top())
		{
		case ITEMTYPE_STATEMENT:
			// Nothing to pop
			break;

		case ITEMTYPE_STRING:
			if(!Strings.empty())
				Strings.pop();
			break;

		case ITEMTYPE_STRINGLITERAL:
			StringLiterals.pop();
			break;

		case ITEMTYPE_INTEGERLITERAL:
			IntegerLiterals.pop();
			break;

		case ITEMTYPE_BOOLEANLITERAL:
			BooleanLiterals.pop();
			break;

		case ITEMTYPE_REALLITERAL:
			RealLiterals.pop();
			break;

		default:
			throw FatalException("Invalid entry on parser stack");
		}

		PushedItemTypes.pop();
	}
}

//
// Build a sequence of bytecode instructions that binds a reference to the assignment target
//
// This function wraps both individual-variable assignments and nested structure member assignments.
//
void CompilationSemantics::AssignmentTarget::EmitReferenceBindings(ByteCodeEmitter& emitter) const
{
	emitter.BindReference(Variable);

	for(StringHandles::const_iterator iter = Members.begin(); iter != Members.end(); ++iter)
		emitter.BindStructureReference(*iter);
}

//
// Build a sequence of bytecode instructions that emits the value of the assignment target
//
// Used for op-assignments and chained assignments
//
void CompilationSemantics::AssignmentTarget::EmitCurrentValue(ByteCodeEmitter& emitter, const ScopeDescription& activescope, const StructureDefinitionMap& structures, const StructureNameMap& structurenames, StringPoolManager& stringpool) const
{
	emitter.PushVariableValue(Variable, activescope.GetVariableTypeByID(Variable));

	if(!Members.empty())
	{
		VM::EpochTypeID structuretype = activescope.GetVariableTypeByID(Variable);

		for(StringHandles::const_iterator memberiter = Members.begin(); memberiter != Members.end(); ++memberiter)
		{
			StringHandle structurename = 0;
			for(StructureNameMap::const_iterator iter = structurenames.begin(); iter != structurenames.end(); ++iter)
			{
				if(iter->second == structuretype)
				{
					structurename = iter->first;
					break;
				}
			}

			if(!structurename)
				throw FatalException("Invalid structure ID");

			StringHandle overloadidhandle = stringpool.Pool(L".@@" + stringpool.GetPooledString(structurename) + L"@@" + stringpool.GetPooledString(*memberiter));		
			emitter.PushStringLiteral(*memberiter);
			emitter.Invoke(overloadidhandle);

			structuretype = structures.find(structuretype)->second.GetMemberType(structures.find(structuretype)->second.FindMember(*memberiter));
		}
	}
}


//-------------------------------------------------------------------------------
// Safety/debug checks
//-------------------------------------------------------------------------------

//
// Exception throwing wrapper
//
// We wrap certain exceptions in boost::spirit's exception type so they can be caught by the guard
// clauses provided in the grammar itself. This allows us to throw internal errors that cause the
// parser to skip certain tokens and resume parsing in a new position, for instance.
//
template <typename ExceptionT>
void CompilationSemantics::Throw(const ExceptionT& exception) const
{
	boost::spirit::classic::throw_<ExceptionT, PosIteratorT>(ParsePosition, exception);
}

//
// Perform a simple set of sanity checks to make sure the parser state is consistent and cleaned up
//
// Invoke this after each parsing pass to ensure that we didn't leave something on the parser stack
// accidentally, or otherwise lose track of something important during the parse process.
//
void CompilationSemantics::SanityCheck() const
{
	if(!Strings.empty() || !EntityTypeTags.empty() || !IntegerLiterals.empty() || !StringLiterals.empty() || !FunctionSignatureStack.empty()
	|| !StatementNames.empty() || !StatementParamCount.empty() || !StatementTypes.empty() || !LexicalScopeStack.empty() || !CompileTimeParameters.empty()
	|| !CurrentEntities.empty() || !PendingEmissionBuffers.empty() || !PendingEmitters.empty() || !AssignmentTargets.empty()
	|| !PushedItemTypes.empty() || !InfixOperators.empty() || !UnaryOperators.empty()
	|| !BooleanLiterals.empty() || !HigherOrderFunctionSignatures.empty() || !StructureMembers.empty() || !StructureFunctionSignatures.empty())
	{
		throw FatalException("Parser leaked a resource");
	}
}
