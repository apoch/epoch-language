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
#include "Compiler/Exceptions.h"

#include "Metadata/FunctionSignature.h"

#include "Utility/Strings.h"

#include "Utility/Files/FilesAndPaths.h"

#include <boost/spirit/include/classic_exceptions.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

#include <sstream>
#include <iostream>


// Handy type shortcuts
typedef boost::spirit::classic::position_iterator<const char*> PosIteratorT;
typedef boost::spirit::classic::parser_error<TypeMismatchException, PosIteratorT> SpiritCompatibleTypeMismatchException;


//-------------------------------------------------------------------------------
// Storage of parsed tokens
//-------------------------------------------------------------------------------

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
// Entity type tags are used to help generate prolog and epilog code for each entity,
// as well as track boundaries between execution contexts for entities that trigger
// them (such as hardware acceleration, multithreading, etc.).
//
void CompilationSemantics::StoreEntityType(Bytecode::EntityTag typetag)
{
	switch(typetag)
	{
	//
	// The entity is a function. In this case, we need to track the
	// identifier of the function itself, and if applicable generate
	// the prolog code during the compilation pass.
	//
	case Bytecode::EntityTags::Function:
		CurrentEntities.push(Strings.top());
		if(!IsPrepass)
			EmitterStack.top()->EnterFunction(LexicalScopeStack.top());
		Strings.pop();
		PushedItemTypes.pop();
		break;

	case Bytecode::EntityTags::FreeBlock:
		{
			std::wstring anonymousname = AllocateAnonymousScopeName();
			StringHandle anonymousnamehandle = Session.StringPool.Pool(anonymousname);
			CurrentEntities.push(anonymousname);
			if(!IsPrepass)
				EmitterStack.top()->EnterEntity(Bytecode::EntityTags::FreeBlock, anonymousnamehandle);
			LexicalScopeDescriptions.insert(std::make_pair(anonymousnamehandle, ScopeDescription(&LexicalScopeDescriptions.find(LexicalScopeStack.top())->second)));
			LexicalScopeStack.push(anonymousnamehandle);
		}
		break;

	//
	// Handle the error case where the type tag is not recognized. Non-fatal.
	//
	default:
		throw RecoverableException("Invalid entity type tag");
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
		EmitterStack.top()->InvokeMetacontrol(EntityTypeTags.top());

	EntityTypeTags.pop();
	if(!IsPrepass)
		StatementTypes.pop();

	// We also need to store off the code generated by the entity
	StoreEntityCode();
}

//
// Flag the end of an entity code block and update parser state accordingly
//
// This routine emits any necessary epilog code for the entity, as well as shifts the internal
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
		// The entity being processed is a function. In this case we need to generate some epilog
		// code to handle return values for functions that are not void. Should we fail to look
		// up the function signature of the entity we are exiting, we must raise a fatal error.
		//
		case Bytecode::EntityTags::Function:
			{
				FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(Session.StringPool.Pool(CurrentEntities.top()));
				if(iter == Session.FunctionSignatures.end())
					throw FatalException("Failed to locate function being finalized");

				if(iter->second.GetReturnType() != VM::EpochType_Void)
				{
					EmitterStack.top()->SetReturnRegister(FunctionReturnVars.top());
					FunctionReturnVars.pop();
					FunctionReturnTypeNames.pop();
				}

				EmitterStack.top()->ExitFunction();
			}
			break;

		//
		// The entity being processed is a free-standing code block.
		//
		case Bytecode::EntityTags::FreeBlock:
			EmitterStack.top()->ExitEntity();
			break;

		//
		// Error case: the entity tag is not recognized. Fatal.
		//
		case Bytecode::EntityTags::Invalid:
			throw FatalException("Invalid entity type tag");

		//
		// The entity tag must be a custom entity.
		//
		default:
			EmitterStack.top()->ExitEntity();
		}
	}

	CurrentEntities.pop();
	EntityTypeTags.pop();
	LexicalScopeStack.pop();
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
	if(!IsPrepass)
	{
		std::wstring infixstatementname = StatementNames.top();
		StringHandle infixstatementnamehandle = Session.StringPool.Pool(infixstatementname);

		RemapFunctionToOverload(CompileTimeParameters.top(), StatementParamCount.top(), TypeVector(), infixstatementname, infixstatementnamehandle);

		if(!StatementNames.empty())
		{
			FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(infixstatementnamehandle);
			if(iter == Session.FunctionSignatures.end())
				throw FatalException("Unknown statement, cannot complete parsing");

			StatementTypes.push(iter->second.GetReturnType());
		}
		else
		{
			// We are in a special location such as the initializer of a return value
			FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(infixstatementnamehandle);
			if(iter == Session.FunctionSignatures.end())
				throw FatalException("Unknown statement, cannot complete parsing");

			StatementTypes.push(iter->second.GetReturnType());
		}

		InfixOperators.top().push_back(infixstatementnamehandle);
	}

	StatementNames.pop();
	StatementParamCount.pop();
	PushedItemTypes.push(ITEMTYPE_STATEMENT);
}

//
// Finalize an infix expression
//
void CompilationSemantics::FinalizeInfix()
{
	if(!IsPrepass)
	{
		CollapseUnaryOperators();

		if(!InfixOperators.empty() && !InfixOperators.top().empty())
		{
			CompileTimeParameterVector flatoperandlist;

			for(CompileTimeParameterVector::const_iterator operanditer = CompileTimeParameters.top().begin() + StatementParamCount.top(); operanditer != CompileTimeParameters.top().end(); ++operanditer)
				flatoperandlist.push_back(*operanditer);

			if(!flatoperandlist.empty())
			{
				size_t numoperandscollapsed = flatoperandlist.size();

				for(PrecedenceTable::const_reverse_iterator precedenceiter = Session.OperatorPrecedences.rbegin(); precedenceiter != Session.OperatorPrecedences.rend(); ++precedenceiter)
				{
					int precedence = precedenceiter->first;
					bool collapsed;
					do
					{
						collapsed = false;

						CompileTimeParameterVector::iterator operanditer = flatoperandlist.begin();
						for(StringHandles::iterator operatoriter = InfixOperators.top().begin(); operatoriter != InfixOperators.top().end(); ++operatoriter)
						{
							if(GetOperatorPrecedence(*operatoriter) == precedence)
							{
								ByteBuffer buffer;
								ByteCodeEmitter emitter(buffer);

								CompileTimeParameterVector::iterator firstoperanditer = operanditer;
								VM::EpochTypeID op1type = GetEffectiveType(*operanditer);

								EmitInfixOperand(emitter, *operanditer);
								++operanditer;
								VM::EpochTypeID op2type = GetEffectiveType(*operanditer);
								EmitInfixOperand(emitter, *operanditer);

								CompileTimeParameterVector::iterator secondoperanditer = operanditer;
								++operanditer;
								emitter.Invoke(*operatoriter);

								VerifyInfixOperandTypes(*operatoriter, op1type, op2type);

								firstoperanditer->Type = VM::EpochType_Expression;
								firstoperanditer->ExpressionContents.swap(buffer);

								flatoperandlist.erase(secondoperanditer);
								InfixOperators.top().erase(operatoriter);

								collapsed = true;
								break;
							}
							else
								++operanditer;
						}
					} while(collapsed);
				}

				CompileTimeParameters.top().erase(CompileTimeParameters.top().end() - numoperandscollapsed, CompileTimeParameters.top().end());

				CompileTimeParameter ctparam(L"@@infixresult", VM::EpochType_Expression);
				ctparam.ExpressionType = StatementTypes.top();
				ctparam.ExpressionContents.swap(flatoperandlist[0].ExpressionContents);
				CompileTimeParameters.top().push_back(ctparam);
			}
		}
	}
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

			EmitInfixOperand(emitter, CompileTimeParameters.top()[ctparamindex]);
			
			StringHandle outhandle = iter->first;
			std::wstring outname = Session.StringPool.GetPooledString(outhandle);
			CompileTimeParameterVector paramvec(1, CompileTimeParameters.top()[ctparamindex]);
			RemapFunctionToOverload(paramvec, 0, TypeVector(), outname, outhandle);
			emitter.Invoke(outhandle);

			CompileTimeParameter ctparam(L"@@unaryresult", VM::EpochType_Expression);
			ctparam.ExpressionType = GetEffectiveType(CompileTimeParameters.top()[ctparamindex]);
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
void CompilationSemantics::RegisterPreOperator(const std::wstring& identifier)
{
	if(!IsPrepass)
		TemporaryString = identifier;
}

//
// Register the operand of a pre-operator expression
//
void CompilationSemantics::RegisterPreOperand(const std::wstring& identifier)
{
	if(!IsPrepass)
	{
		std::wstring operatorname = TemporaryString;
		StringHandle operatornamehandle = Session.StringPool.Pool(operatorname);

		StringHandle operandhandle = Session.StringPool.Pool(identifier);

		VM::EpochTypeID variabletype = LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.GetVariableTypeByID(operandhandle);

		CompileTimeParameterVector ctparams;
		ctparams.push_back(CompileTimeParameter(L"@@preoperand", VM::EpochType_Identifier));
		RemapFunctionToOverload(ctparams, 0, TypeVector(1, variabletype), operatorname, operatornamehandle);

		ByteBuffer buffer;
		ByteCodeEmitter emitter(buffer);

		emitter.PushStringLiteral(operandhandle);
		emitter.Invoke(operatornamehandle);
		emitter.AssignVariable(operandhandle);
		emitter.PushVariableValue(operandhandle);

		StatementTypes.push(variabletype);

		if(!CompileTimeParameters.empty())
		{
			CompileTimeParameter ctparam(L"@@preoperation", VM::EpochType_Expression);
			ctparam.ExpressionType = VM::EpochType_Integer;
			ctparam.ExpressionContents.swap(buffer);
			CompileTimeParameters.top().push_back(ctparam);
		}
		else
			EmitterStack.top()->EmitBuffer(buffer);
	}

	PushedItemTypes.push(ITEMTYPE_STATEMENT);
}

//
// Register the operator of a post-operator expression
//
void CompilationSemantics::RegisterPostOperator(const std::wstring& identifier)
{
	if(!IsPrepass)
	{
		std::wstring operatorname = identifier;
		StringHandle operatornamehandle = Session.StringPool.Pool(operatorname);

		StringHandle operandhandle = Session.StringPool.Pool(TemporaryString);

		VM::EpochTypeID variabletype = LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.GetVariableTypeByID(operandhandle);

		CompileTimeParameterVector ctparams;
		ctparams.push_back(CompileTimeParameter(L"@@preoperand", VM::EpochType_Identifier));
		RemapFunctionToOverload(ctparams, 0, TypeVector(1, variabletype), operatorname, operatornamehandle);

		ByteBuffer buffer;
		ByteCodeEmitter emitter(buffer);

		emitter.PushVariableValue(operandhandle);
		emitter.PushStringLiteral(operandhandle);
		emitter.Invoke(operatornamehandle);
		emitter.AssignVariable(operandhandle);

		StatementTypes.push(variabletype);

		if(!CompileTimeParameters.empty())
		{
			CompileTimeParameter ctparam(L"@@preoperation", VM::EpochType_Expression);
			ctparam.ExpressionType = VM::EpochType_Integer;
			ctparam.ExpressionContents.swap(buffer);
			CompileTimeParameters.top().push_back(ctparam);
		}
		else
			EmitterStack.top()->EmitBuffer(buffer);
	}

	PushedItemTypes.push(ITEMTYPE_STATEMENT);
}

//
// Register the operand of a post-operator expression
//
void CompilationSemantics::RegisterPostOperand(const std::wstring& identifier)
{
	if(!IsPrepass)
		TemporaryString = identifier;
}


//-------------------------------------------------------------------------------
// Entity parameter definitions
//-------------------------------------------------------------------------------

//
// Begin parsing the definition list of parameters passed to an entity (usually a function)
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
		for(std::list<std::pair<boost::spirit::classic::position_iterator<const char*>, StringHandle> >::const_iterator iter = OverloadDefinitions.begin(); iter != OverloadDefinitions.end(); ++iter)
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
}

//
// Register the name of a function/entity parameter
//
// Adds the parameter to the entity's signature, and optionally in the compilation phase
// adds the corresponding variable to the entity's local lexical scope.
//
void CompilationSemantics::RegisterParameterName(const std::wstring& name)
{
	FunctionSignatureStack.top().AddParameter(name, ParamType);
	
	if(!IsPrepass)
	{
		ScopeMap::iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
		if(iter == LexicalScopeDescriptions.end())
			throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");
		
		iter->second.AddVariable(name, Session.StringPool.Pool(name), ParamType, VARIABLE_ORIGIN_PARAMETER);
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
		
		iter->second.AddVariable(L"@@patternmatched", Session.StringPool.Pool(L"@@patternmatched"), paramtype, VARIABLE_ORIGIN_PARAMETER);
	}
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
// be within the entity itself as prolog instructions; this also ensures that the initializer
// is called correctly every time the entity is invoked. To enforce this we redirect all code
// emission to a temporary buffer and then flush that buffer into the parent stream after all
// prolog code has been generated.
//
void CompilationSemantics::BeginReturnSet()
{
	ReturnsIncludedStatement.push(false);
	if(!IsPrepass)
	{
		PendingEmissionBuffers.push(ByteBuffer());
		PendingEmitters.push(ByteCodeEmitter(PendingEmissionBuffers.top()));
		EmitterStack.push(&PendingEmitters.top());

		InfixOperators.push(StringHandles());
		UnaryOperators.push(UnaryOperatorVector());
		CompileTimeParameters.push(CompileTimeParameterVector());
		StatementParamCount.push(0);
	}
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
	if(ReturnsIncludedStatement.top())
	{
		if(!IsPrepass)
			CompileTimeParameters.c.clear();
	}

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
		EmitPendingCode();
		EmitterStack.pop();

		InfixOperators.pop();
		UnaryOperators.pop();
		CompileTimeParameters.c.clear();
		StatementParamCount.pop();
	}

	FunctionSignatureStack.pop();
	ReturnsIncludedStatement.pop();
}

//
// Track the return type of an entity, if it is not void
//
void CompilationSemantics::RegisterReturnType(const std::wstring& type)
{
	if(!IsPrepass)
		FunctionReturnTypeNames.push(type);
	FunctionSignatureStack.top().SetReturnType(LookupTypeName(type));
}

//
// Track the name of an entity's return value
//
// This identifier is used to create a local variable within the entity's lexical
// scope that holds the entity's return value during execution. Note that we also
// emit an instruction tagging the identifier for construction in the prolog code
// of the entity itself.
//
void CompilationSemantics::RegisterReturnName(const std::wstring& name)
{
	if(!IsPrepass)
	{
		StringHandle namehandle = Session.StringPool.Pool(name);
		FunctionReturnVars.push(namehandle);
		PendingEmitters.top().PushStringLiteral(namehandle);
		LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.AddVariable(name, namehandle, FunctionSignatureStack.top().GetReturnType(), VARIABLE_ORIGIN_RETURN);
	}
}

//
// Register that the expression initializing an entity's return value has been fully parsed
//
// Once the initializer is parsed, we need to check the parsed expression for validity (i.e. type
// consistency) as well as emit construction code during the compilation phase. We also need some
// additional housekeeping work done to keep the parser state tidy.
//
void CompilationSemantics::RegisterReturnValue()
{
	// Shift output into a temporary buffer so it can be injected at the top of the entity's code
	if(!IsPrepass)
	{
		PendingEmissionBuffers.push(ByteBuffer());
		PendingEmitters.push(ByteCodeEmitter(PendingEmissionBuffers.top()));
	}

	// Perform type validation and housekeeping
	switch(FunctionSignatureStack.top().GetReturnType())
	{
	case VM::EpochType_Integer:
		if(PushedItemTypes.top() == ITEMTYPE_INTEGERLITERAL)
		{
			if(!IsPrepass)
				PendingEmitters.top().PushIntegerLiteral(IntegerLiterals.top());
			IntegerLiterals.pop();
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STRING)
		{
			StringHandle varhandle = Session.StringPool.Pool(Strings.top());
			Strings.pop();

			VM::EpochTypeID vartype = FunctionSignatureStack.top().GetParameter(Session.StringPool.GetPooledString(varhandle)).Type;
			if(vartype != VM::EpochType_Integer)
				Throw(TypeMismatchException("The function is defined as returning an integer but the provided default return value is not of an integer type."));

			if(!IsPrepass)
				PendingEmitters.top().PushVariableValue(varhandle);
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STATEMENT)
		{
			if(!IsPrepass)
			{
				if(StatementTypes.top() != VM::EpochType_Integer)
					Throw(TypeMismatchException("The function is defined as returning an integer but the provided default return value is not of an integer type."));
			}

			ReturnsIncludedStatement.top() = true;
		}
		else
			Throw(TypeMismatchException("The function is defined as returning an integer but the provided default return value is not of an integer type."));
		break;

	case VM::EpochType_String:
		if(PushedItemTypes.top() == ITEMTYPE_STRINGLITERAL)
		{
			if(!IsPrepass)
				PendingEmitters.top().PushStringLiteral(StringLiterals.top());
			StringLiterals.pop();
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STRING)
		{
			StringHandle varhandle = Session.StringPool.Pool(Strings.top());
			Strings.pop();

			VM::EpochTypeID vartype = FunctionSignatureStack.top().GetParameter(Session.StringPool.GetPooledString(varhandle)).Type;
			if(vartype != VM::EpochType_String)
				Throw(TypeMismatchException("The function is defined as returning a string but the provided default return value is not of string type."));

			if(!IsPrepass)
				PendingEmitters.top().PushVariableValue(varhandle);
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STATEMENT)
		{
			if(!IsPrepass)
			{
				if(StatementTypes.top() != VM::EpochType_String)
					Throw(TypeMismatchException("The function is defined as returning a string but the provided default return value is not of string type."));
			}

			ReturnsIncludedStatement.top() = true;
		}
		else
			Throw(TypeMismatchException("The function is defined as returning a string but the provided default return value is not of string type."));
		break;

	case VM::EpochType_Boolean:
		if(PushedItemTypes.top() == ITEMTYPE_BOOLEANLITERAL)
		{
			if(!IsPrepass)
				PendingEmitters.top().PushBooleanLiteral(BooleanLiterals.top());
			BooleanLiterals.pop();
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STRING)
		{
			StringHandle varhandle = Session.StringPool.Pool(Strings.top());
			Strings.pop();

			VM::EpochTypeID vartype = FunctionSignatureStack.top().GetParameter(Session.StringPool.GetPooledString(varhandle)).Type;
			if(vartype != VM::EpochType_Boolean)
				Throw(TypeMismatchException("The function is defined as returning a boolean but the provided default return value is not of a boolean type."));

			if(!IsPrepass)
				PendingEmitters.top().PushVariableValue(varhandle);
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STATEMENT)
		{
			if(!IsPrepass)
			{
				if(StatementTypes.top() != VM::EpochType_Boolean)
					Throw(TypeMismatchException("The function is defined as returning a boolean but the provided default return value is not of a boolean type."));
			}

			ReturnsIncludedStatement.top() = true;
		}
		else
			Throw(TypeMismatchException("The function is defined as returning a boolean but the provided default return value is not of a boolean type."));
		break;

	case VM::EpochType_Real:
		if(PushedItemTypes.top() == ITEMTYPE_REALLITERAL)
		{
			if(!IsPrepass)
				PendingEmitters.top().PushRealLiteral(RealLiterals.top());
			RealLiterals.pop();
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STRING)
		{
			StringHandle varhandle = Session.StringPool.Pool(Strings.top());
			Strings.pop();

			VM::EpochTypeID vartype = FunctionSignatureStack.top().GetParameter(Session.StringPool.GetPooledString(varhandle)).Type;
			if(vartype != VM::EpochType_Real)
				Throw(TypeMismatchException("The function is defined as returning a real but the provided default return value is not of a real type."));

			if(!IsPrepass)
				PendingEmitters.top().PushVariableValue(varhandle);
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STATEMENT)
		{
			if(!IsPrepass)
			{
				if(StatementTypes.top() != VM::EpochType_Real)
					Throw(TypeMismatchException("The function is defined as returning a real but the provided default return value is not of a real type."));
			}

			ReturnsIncludedStatement.top() = true;
		}
		else
			Throw(TypeMismatchException("The function is defined as returning a real but the provided default return value is not of a real type."));
		break;

	default:
		throw NotImplementedException("Unsupported function return type in CompilationSemantics::RegisterReturnValue");
	}

	// Emit code to invoke the type constructor for the return variable
	if(!IsPrepass)
		PendingEmitters.top().Invoke(Session.StringPool.Pool(FunctionReturnTypeNames.top()));

	// Final cleanup
	StatementTypes.c.clear();
	PushedItemTypes.pop();
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
	std::wstring statementname = StatementNames.top();
	StringHandle statementnamehandle = Session.StringPool.Pool(statementname);

	StatementNames.pop();
	StatementParamCount.pop();
	PushedItemTypes.push(ITEMTYPE_STATEMENT);

	if(!IsPrepass)
	{
		InfixOperators.pop();
		UnaryOperators.pop();

		TypeVector outerexpectedtypes = WalkCallChainForExpectedTypes(StatementNames.size() - 1);
		RemapFunctionToOverload(CompileTimeParameters.top(), 0, outerexpectedtypes, statementname, statementnamehandle);

		FunctionCompileHelperTable::const_iterator fchiter = CompileTimeHelpers.find(statementname);
		if(fchiter != CompileTimeHelpers.end())
			fchiter->second(GetLexicalScopeDescription(LexicalScopeStack.top()), CompileTimeParameters.top());

		FunctionSignatureSet::const_iterator fsiter = Session.FunctionSignatures.find(statementnamehandle);
		if(fsiter == Session.FunctionSignatures.end())
			Throw(RecoverableException("The function \"" + narrow(statementname) + "\" is not defined in this scope"));

		// Validate the parameters we've been given against the parameters to this overload
		if(CompileTimeParameters.top().size() == fsiter->second.GetNumParameters())
		{
			for(size_t i = 0; i < CompileTimeParameters.top().size(); ++i)
			{
				switch(CompileTimeParameters.top()[i].Type)
				{
				case VM::EpochType_Error:
					throw FatalException("Parameter's type is explicitly flagged as invalid");

				case VM::EpochType_Void:
					Throw(RecoverableException("Parameter has no type; cannot be passed to this function"));

				case VM::EpochType_Identifier:
					if(fsiter->second.GetParameter(i).Type == VM::EpochType_Identifier)
						PendingEmitters.top().PushStringLiteral(CompileTimeParameters.top()[i].Payload.StringHandleValue);
					else
					{
						ScopeMap::const_iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
						if(iter == LexicalScopeDescriptions.end())
							throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");

						if(!iter->second.HasVariable(CompileTimeParameters.top()[i].StringPayload))
							Throw(RecoverableException("No variable by the name \"" + narrow(CompileTimeParameters.top()[i].StringPayload) + "\" was found in this scope"));

						if(iter->second.GetVariableTypeByID(CompileTimeParameters.top()[i].Payload.StringHandleValue) != fsiter->second.GetParameter(i).Type)
							Throw(RecoverableException("The variable \"" + narrow(CompileTimeParameters.top()[i].StringPayload) + "\" has the wrong type to be used for this parameter"));

						PendingEmitters.top().PushVariableValue(CompileTimeParameters.top()[i].Payload.StringHandleValue);
					}
					break;

				case VM::EpochType_Expression:
					if(fsiter->second.GetParameter(i).Type == CompileTimeParameters.top()[i].ExpressionType)
						PendingEmitters.top().EmitBuffer(CompileTimeParameters.top()[i].ExpressionContents);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(RecoverableException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_Integer:
					if(fsiter->second.GetParameter(i).Type == VM::EpochType_Integer)
						PendingEmitters.top().PushIntegerLiteral(CompileTimeParameters.top()[i].Payload.IntegerValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(RecoverableException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_String:
					if(fsiter->second.GetParameter(i).Type == VM::EpochType_String)
						PendingEmitters.top().PushStringLiteral(CompileTimeParameters.top()[i].Payload.StringHandleValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(RecoverableException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_Boolean:
					if(fsiter->second.GetParameter(i).Type == VM::EpochType_Boolean)
						PendingEmitters.top().PushBooleanLiteral(CompileTimeParameters.top()[i].Payload.BooleanValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(RecoverableException(narrow(errormsg.str())));
					}
					break;

				case VM::EpochType_Real:
					if(fsiter->second.GetParameter(i).Type == VM::EpochType_Real)
						PendingEmitters.top().PushRealLiteral(CompileTimeParameters.top()[i].Payload.RealValue);
					else
					{
						std::wostringstream errormsg;
						errormsg << L"Parameter " << (i + 1) << L" to function \"" << statementname << L"\" is of the wrong type";
						Throw(RecoverableException(narrow(errormsg.str())));
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
			errormsg << L"The function \"" << statementname << L"\" expects " << fsiter->second.GetNumParameters() << L" parameters, but ";
			errormsg << CompileTimeParameters.top().size() << L" were provided";
			Throw(RecoverableException(narrow(errormsg.str())));
		}

		StatementTypes.push(fsiter->second.GetReturnType());

		PendingEmitters.top().Invoke(statementnamehandle);

		CompileTimeParameters.pop();
		if(!CompileTimeParameters.empty())
		{
			CompileTimeParameter ctparam(L"@@expression", VM::EpochType_Expression);
			ctparam.ExpressionType = fsiter->second.GetReturnType();
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
	if(!IsPrepass)
	{
		if(!LexicalScopeDescriptions[LexicalScopeStack.top()].HasVariable(TemporaryString))
			Throw(RecoverableException("The variable \"" + narrow(TemporaryString) + "\" is not defined in this scope"));
	}

	StatementNames.push(L"=");
	StatementParamCount.push(0);
	AssignmentTargets.push(Session.StringPool.Pool(TemporaryString));
	TemporaryString.clear();
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
// Begin parsing an assignment that has additional side effects
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
	AssignmentTargets.push(Session.StringPool.Pool(TemporaryString));
	TemporaryString.clear();
	if(!IsPrepass)
	{
		CompileTimeParameter ctparam(L"@@lhs", VM::EpochType_Identifier);
		ctparam.Payload.StringHandleValue = AssignmentTargets.top();
		CompileTimeParameters.push(CompileTimeParameterVector(1, ctparam));
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
			EmitterStack.top()->PushStringLiteral(AssignmentTargets.top());
			
		EmitInfixOperand(*EmitterStack.top(), CompileTimeParameters.top().back());

		if(StatementNames.top() != L"=")
		{
			std::wstring assignmentop = StatementNames.top();
			StringHandle assignmentophandle = Session.StringPool.Pool(assignmentop);
			RemapFunctionToOverload(CompileTimeParameters.top(), 0, TypeVector(1, expressiontype), assignmentop, assignmentophandle);

			EmitterStack.top()->Invoke(assignmentophandle);
		}

		PendingEmitters.pop();
		PendingEmissionBuffers.pop();
		InfixOperators.pop();
		UnaryOperators.pop();

		EmitterStack.top()->AssignVariable(AssignmentTargets.top());
		CompileTimeParameters.pop();
		StatementTypes.c.clear();

		// Check for chained assignments
		if(AssignmentTargets.size() > 1)
		{
			CompileTimeParameter ctparam(L"@@chainedassignment", VM::EpochType_Identifier);
			ctparam.Payload.StringHandleValue = AssignmentTargets.top();
			CompileTimeParameters.top().push_back(ctparam);
		}

		if(expressiontype != LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.GetVariableTypeByID(AssignmentTargets.top()))
		{
			AssignmentTargets.pop();
			StatementParamCount.pop();
			Throw(TypeMismatchException("Right hand side of assignment does not match variable type"));
		}
	}
	StatementNames.pop();
	AssignmentTargets.pop();
	StatementParamCount.pop();
}


//-------------------------------------------------------------------------------
// Entity invocation
//-------------------------------------------------------------------------------

//
// Signal the beginning of a set of parameters passed to an entity invocation
//
void CompilationSemantics::BeginEntityParams()
{
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
			Throw(RecoverableException("Incorrect parameters to " + narrow(entityname)));
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
	if(!StatementParamCount.empty())
	{
		++StatementParamCount.top();
		PushParam(L"@@passedparam");
	}

	if(!IsPrepass)
	{
		CollapseUnaryOperators();
		InfixOperators.top().clear();
		UnaryOperators.top().clear();
		PendingEmissionBuffers.top().clear();
	}
}

//
// Queue a parameter passed to a statement for later validation, incrementing the passed parameter count as we go
//
void CompilationSemantics::PushInfixParam()
{
	if(!StatementParamCount.empty())
	{
		++StatementParamCount.top();
		PushParam(L"@@infixoperand");
	}
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

//
// Helper routine for type-checking parameters
//
bool CompilationSemantics::CheckParameterValidity(VM::EpochTypeID expectedtype)
{
	if(PushedItemTypes.empty())
		throw ParameterException("Expected at least one parameter here, but none have been provided");

	switch(PushedItemTypes.top())
	{
	case ITEMTYPE_STATEMENT:
		if(StatementTypes.empty())
			return false;
		else if(StatementTypes.top() != expectedtype)
		{
			StatementTypes.pop();
			return false;
		}
		break;

	case ITEMTYPE_STRING:
		if(expectedtype != VM::EpochType_Identifier)
		{
			if(GetLexicalScopeDescription(LexicalScopeStack.top()).HasVariable(Strings.top()))
				return (GetLexicalScopeDescription(LexicalScopeStack.top()).GetVariableTypeByID(Session.StringPool.Pool(Strings.top())) == expectedtype);

			return false;
		}
		break;

	case ITEMTYPE_STRINGLITERAL:
		if(expectedtype != VM::EpochType_String)
			return false;
		break;
	
	case ITEMTYPE_INTEGERLITERAL:
		if(expectedtype != VM::EpochType_Integer)
			return false;
		break;

	case ITEMTYPE_BOOLEANLITERAL:
		if(expectedtype != VM::EpochType_Boolean)
			return false;
		break;

	default:
		throw FatalException("Unrecognized ItemType value in LastPushedItemType, parser is probably broken");
	}

	return true;
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
				EmitterStack.top()->LexicalScopeEntry(Session.StringPool.Pool(iter->second.GetParameter(i).Name), iter->second.GetParameter(i).Type, VARIABLE_ORIGIN_PARAMETER);

			EmitterStack.top()->EnterPatternResolver(iter->first);
			std::pair<AllOverloadsMap::const_iterator, AllOverloadsMap::const_iterator> range = OriginalFunctionsForPatternResolution.equal_range(iter->first);
			for(AllOverloadsMap::const_iterator originalfunciter = range.first; originalfunciter != range.second; ++originalfunciter)
				EmitterStack.top()->ResolvePattern(originalfunciter->second, Session.FunctionSignatures.find(originalfunciter->second)->second);
			EmitterStack.top()->ExitPatternResolver();
		}

		for(std::map<StringHandle, std::wstring>::const_iterator iter = Session.StringPool.GetInternalPool().begin(); iter != Session.StringPool.GetInternalPool().end(); ++iter)
			EmitterStack.top()->PoolString(iter->first, iter->second);

		for(ScopeMap::const_iterator iter = LexicalScopeDescriptions.begin(); iter != LexicalScopeDescriptions.end(); ++iter)
		{
			EmitterStack.top()->DefineLexicalScope(iter->first, FindLexicalScopeName(iter->second.ParentScope), iter->second.GetVariableCount());
			for(size_t i = 0; i < iter->second.GetVariableCount(); ++i)
				EmitterStack.top()->LexicalScopeEntry(Session.StringPool.Pool(iter->second.GetVariableName(i)), iter->second.GetVariableTypeByIndex(i), iter->second.GetVariableOrigin(i));
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
void CompilationSemantics::RemapFunctionToOverload(const CompileTimeParameterVector& params, size_t paramoffset, const TypeVector& possiblereturntypes, std::wstring& out_remappedname, StringHandle& out_remappednamehandle) const
{
	StringVector matchingnames;
	StringHandles matchingnamehandles;

	GetAllMatchingOverloads(params, paramoffset, possiblereturntypes, out_remappedname, out_remappednamehandle, matchingnames, matchingnamehandles);

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


void CompilationSemantics::GetAllMatchingOverloads(const CompileTimeParameterVector& params, size_t paramoffset, const TypeVector& possiblereturntypes, const std::wstring& originalname, StringHandle originalnamehandle, StringVector& out_names, StringHandles& out_namehandles) const
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

		if((params.size() - paramoffset) > signatureiter->second.GetNumParameters())
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
		for(size_t i = paramoffset; i < params.size(); ++i)
		{
			if(signatureiter->second.GetParameter(i - paramoffset).Name == L"@@patternmatched")
			{
				patternmatching = true;
				switch(params[i].Type)
				{
				case VM::EpochType_Integer:
					if(signatureiter->second.GetParameter(i - paramoffset).Payload.IntegerValue != params[i].Payload.IntegerValue)
						patternsucceeded = false;
					break;

				// If an identifier is passed, we have to check if the overload wants an identifier or a variable
				// If a variable is preferred, we always delegate to the runtime match default
				case VM::EpochType_Identifier:
					if(signatureiter->second.GetParameter(i - paramoffset).Type == VM::EpochType_Identifier)
					{
						if(signatureiter->second.GetParameter(i - paramoffset).Payload.StringHandleValue != params[i].Payload.StringHandleValue)
							patternsucceeded = false;
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

						if(!iter->second.HasVariable(params[i].StringPayload))
							Throw(RecoverableException("No variable by the name \"" + narrow(params[i].StringPayload) + "\" was found in this scope"));

						if(iter->second.GetVariableTypeByID(params[i].Payload.StringHandleValue) != signatureiter->second.GetParameter(i - paramoffset).Type)
						{
							patternsucceeded = false;
							matched = false;
							break;
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
					matches.insert(*iter);
					break;
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
	else if(type == L"string")
		return VM::EpochType_String;
	else if(type == L"boolean")
		return VM::EpochType_Boolean;
	else if(type == L"real")
		return VM::EpochType_Real;

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
		return TypeVector(1, LexicalScopeDescriptions.find(LexicalScopeStack.top())->second.GetVariableTypeByID(AssignmentTargets.top()));
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
		GetAllMatchingOverloads(CompileTimeParameters.c.at(index), paramindex, outerexpectedtypes, name, namehandle, outnames, outnamehandles);

		TypeVector ret;

		for(size_t i = 0; i < outnames.size(); ++i)
		{
			FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(outnamehandles[i]);
			if(iter == Session.FunctionSignatures.end())
				Throw(RecoverableException("The function \"" + narrow(name) + "\" is not defined in this scope"));

			ret.push_back(iter->second.GetParameter(paramindex).Type);
		}

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
		emitter.PushIntegerLiteral(ctparam.Payload.StringHandleValue);
		break;

	case VM::EpochType_Boolean:
		emitter.PushBooleanLiteral(ctparam.Payload.BooleanValue);
		break;

	case VM::EpochType_Real:
		emitter.PushRealLiteral(ctparam.Payload.RealValue);
		break;

	case VM::EpochType_Identifier:
		emitter.PushVariableValue(ctparam.Payload.StringHandleValue);
		break;

	case VM::EpochType_Expression:
		emitter.EmitBuffer(ctparam.ExpressionContents);
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
	|| !CurrentEntities.empty() || !FunctionReturnVars.empty() || !PendingEmissionBuffers.empty() || !PendingEmitters.empty() || !AssignmentTargets.empty()
	|| !PushedItemTypes.empty() || !ReturnsIncludedStatement.empty() || !FunctionReturnTypeNames.empty() || !InfixOperators.empty() || !UnaryOperators.empty()
	|| !BooleanLiterals.empty())
		throw FatalException("Parser leaked a resource");
}
