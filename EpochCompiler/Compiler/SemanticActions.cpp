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

#include <boost/spirit/include/classic_exceptions.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

#include <sstream>


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
		// Error case: the entity tag is not recognized. Fatal.
		//
		default:
			throw FatalException("Invalid entity type tag");
		}
	}

	EntityTypeTags.pop();
	CurrentEntities.pop();
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
	unsigned paramindex;

	if(StatementParamCount.empty())
	{
		// We're in a function return value initializer
		paramindex = 0;
	}
	else
	{
		// Standard case of infix expression within a statement
		paramindex = StatementParamCount.top();
	}

	StatementNames.push(identifier);
	StatementParamCount.push(0);

	if(!IsPrepass)
		CompileTimeParameters.push(std::vector<CompileTimeParameter>());

	ValidateAndPushParam(paramindex);
}

//
// Record the end of an infix expression
//
// This may not be the actual end of the expression per se, but rather simply signals
// that both terms and the infix operator itself have all been parsed and saved off into
// the state data. Note that the current implementation does NOT respect precedence rules
// and therefore should be burned at the earliest possible convenience and replaced with
// something less craptacular.
//
void CompilationSemantics::CompleteInfix()
{
	// TODO - reimplement support for infix operator precedence rules (then document it)
	std::wstring infixstatementname = StatementNames.top();
	StringHandle infixstatementnamehandle = Session.StringPool.Pool(infixstatementname);
	unsigned infixparamcount = StatementParamCount.top();

	StatementNames.pop();
	StatementParamCount.pop();
	PushedItemTypes.push(ITEMTYPE_STATEMENT);

	if(!StatementNames.empty())
	{
		if(!IsPrepass)
		{
			std::wstring statementname = StatementNames.top();

			FunctionCompileHelperTable::const_iterator iter = CompileTimeHelpers.find(statementname);
			if(iter != CompileTimeHelpers.end())
				iter->second(GetLexicalScopeDescription(LexicalScopeStack.top()), CompileTimeParameters.top());

			CompileTimeParameters.pop();
			if(!CompileTimeParameters.empty())
			{
				FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(infixstatementnamehandle);
				if(iter == Session.FunctionSignatures.end())
					throw FatalException("Unknown statement, cannot complete parsing");

				StatementTypes.push(iter->second.GetReturnType());

				if(statementname == L"=")
					CompileTimeParameters.top().push_back(CompileTimeParameter(L"rhs", VM::EpochType_Integer));		// TODO - check type of LHS
				else
				{
					iter = Session.FunctionSignatures.find(Session.StringPool.Pool(statementname));
					if(iter == Session.FunctionSignatures.end())
						throw FatalException("Unknown statement, cannot complete parsing");

					const std::wstring& paramname = iter->second.GetParameterName(StatementParamCount.top());
					VM::EpochTypeID paramtype = iter->second.GetParameterType(StatementParamCount.top());

					CompileTimeParameters.top().push_back(CompileTimeParameter(paramname, paramtype));
				}
			}
		}
	}
	else
	{
		// We are in a special location such as the initializer of a return value
		FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(infixstatementnamehandle);
		if(iter == Session.FunctionSignatures.end())
			throw FatalException("Unknown statement, cannot complete parsing");

		StatementTypes.push(iter->second.GetReturnType());
	}

	if(!IsPrepass)
		EmitterStack.top()->Invoke(infixstatementnamehandle);
}



//-------------------------------------------------------------------------------
// Entity parameter definitions
//-------------------------------------------------------------------------------

//
// Begin parsing the definition list of parameters passed to an entity (usually a function)
//
void CompilationSemantics::BeginParameterSet()
{
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
		std::map<StringHandle, ScopeDescription>::iterator iter = LexicalScopeDescriptions.find(LexicalScopeStack.top());
		if(iter == LexicalScopeDescriptions.end())
			throw FatalException("No lexical scope has been registered for the identifier \"" + narrow(Session.StringPool.GetPooledString(LexicalScopeStack.top())) + "\"");
		
		iter->second.AddVariable(name, Session.StringPool.Pool(name), ParamType, VARIABLE_ORIGIN_PARAMETER);
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
		PendingEmissionBuffers.push(std::vector<Byte>());
		PendingEmitters.push(ByteCodeEmitter(PendingEmissionBuffers.top()));
		EmitterStack.push(&PendingEmitters.top());
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
		Session.FunctionSignatures.insert(std::make_pair(LexicalScopeStack.top(), FunctionSignatureStack.top()));
	else
	{
		EmitPendingCode();
		EmitterStack.pop();
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
		PendingEmissionBuffers.push(std::vector<Byte>());
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
			if(!IsPrepass)
				PendingEmitters.top().PushVariableValue(Session.StringPool.Pool(Strings.top()));
			Strings.pop();
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STATEMENT)
		{
			if(!IsPrepass)
			{
				if(StatementTypes.top() != VM::EpochType_Integer)
					throw TypeMismatchException("The function is defined as returning an integer but the provided default return value is not of an integer type.");
			}

			ReturnsIncludedStatement.top() = true;
		}
		else
			throw TypeMismatchException("The function is defined as returning an integer but the provided default return value is not of an integer type.");
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
		CompileTimeParameters.push(std::vector<CompileTimeParameter>());
		OverloadResolutionFailed.push(false);
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
		if(!OverloadResolutionFailed.top())
		{
			RemapFunctionToOverload(CompileTimeParameters.top(), statementname, statementnamehandle);

			FunctionCompileHelperTable::const_iterator fchiter = CompileTimeHelpers.find(statementname);
			if(fchiter != CompileTimeHelpers.end())
				fchiter->second(GetLexicalScopeDescription(LexicalScopeStack.top()), CompileTimeParameters.top());

			FunctionSignatureSet::const_iterator fsiter = Session.FunctionSignatures.find(statementnamehandle);
			if(fsiter == Session.FunctionSignatures.end())
				Throw(RecoverableException("The function \"" + narrow(statementname) + "\" is not defined in this scope"));

			StatementTypes.push(fsiter->second.GetReturnType());

			CompileTimeParameters.pop();
			if(!CompileTimeParameters.empty())
			{
				const std::wstring& paramname = fsiter->second.GetParameterName(StatementParamCount.top());
				VM::EpochTypeID paramtype = fsiter->second.GetParameterType(StatementParamCount.top());

				CompileTimeParameters.top().push_back(CompileTimeParameter(paramname, paramtype));
			}

			EmitterStack.top()->Invoke(statementnamehandle);
		}
		OverloadResolutionFailed.pop();
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
		StatementTypes.c.clear();

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
		CompileTimeParameters.push(std::vector<CompileTimeParameter>());
}

//
// Finish parsing an assignment operation
//
// This function is invoked once both the left and right sides of the expression have been parsed,
// meaning that we can safely emit the assignment operator invocation code, and clean up.
//
void CompilationSemantics::CompleteAssignment()
{
	StatementNames.pop();
	if(!IsPrepass)
	{
		// TODO - type checking
		EmitterStack.top()->AssignVariable(AssignmentTargets.top());
		CompileTimeParameters.pop();
		StatementTypes.pop();
	}
	AssignmentTargets.pop();
	StatementParamCount.pop();
}


//-------------------------------------------------------------------------------
// Parameter validation
//-------------------------------------------------------------------------------

//
// Validate a parameter passed to a statement, incrementing the passed parameter count as we go
//
void CompilationSemantics::ValidateStatementParam()
{
	if(!StatementParamCount.empty())
	{
		unsigned paramindex = StatementParamCount.top();
		++StatementParamCount.top();
		ValidateAndPushParam(paramindex);
	}
}

//
// Perform the actual type validation of a parameter to a statement, and optionally emit the code
// for passing the parameter itself during the compilation phase.
//
void CompilationSemantics::ValidateAndPushParam(unsigned paramindex)
{
	std::wstring paramname;
	VM::EpochTypeID expectedtype = VM::EpochType_Error;

	if(!IsPrepass)
	{
		// For assignment operations we need to do special handling since the operator is built in
		// and we won't be able to look its parameters up in the entity signature tables
		if(StatementNames.top() == L"=")
		{
			paramname = L"rhs";
			expectedtype = VM::EpochType_Integer;		// TODO - check type of LHS

			CheckParameterValidity(expectedtype);
		}
		else
		{
			bool matchedparam = false;

			StringHandle rawnamehandle = Session.StringPool.Pool(StatementNames.top());
			std::map<StringHandle, std::set<StringHandle> >::const_iterator overloadsiter = FunctionOverloadNames.find(rawnamehandle);
			if(overloadsiter != FunctionOverloadNames.end())
			{
				const std::set<StringHandle>& overloadnames = overloadsiter->second;
				for(std::set<StringHandle>::const_iterator overloaditer = overloadnames.begin(); overloaditer != overloadnames.end(); ++overloaditer)
				{
					StringHandle statementnamehandle = *overloaditer;
					FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(statementnamehandle);
					if(iter == Session.FunctionSignatures.end())
						Throw(RecoverableException("The function \"" + narrow(StatementNames.top()) + "\" is not defined in this scope"));

					paramname = iter->second.GetParameterName(paramindex);
					expectedtype = iter->second.GetParameterType(paramindex);

					// TODO - using exceptions for this is naughty. Rewrite it to just return a validity value from CheckParameterValidity().
					try
					{
						CheckParameterValidity(expectedtype);
						matchedparam = true;
						break;
					}
					catch(SpiritCompatibleTypeMismatchException&)
					{
						if(overloadnames.size() == 1)
							throw;
					}
				}
			}
			else
			{
				// This is probably a library function with no registered overloads
				FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(rawnamehandle);
				if(iter == Session.FunctionSignatures.end())
					Throw(RecoverableException("The function \"" + narrow(StatementNames.top()) + "\" is not defined in this scope"));

				paramname = iter->second.GetParameterName(paramindex);
				expectedtype = iter->second.GetParameterType(paramindex);

				CheckParameterValidity(expectedtype);
				matchedparam = true;
			}

			if(!matchedparam)
			{
				OverloadResolutionFailed.top() = true;
				Failed = true;
				Throw(RecoverableException("No overloaded function takes a matching set of parameters"));
			}
		}
	}

	CompileTimeParameter ctparam(paramname, expectedtype);

	// Emit any necessary code and set up the compile-time parameter payload
	switch(PushedItemTypes.top())
	{
	case ITEMTYPE_STATEMENT:
		// Nothing to do, the nested statement handler has already taken care of things
		break;

	case ITEMTYPE_STRING:
		{
			StringHandle handle = Session.StringPool.Pool(Strings.top());
			if(!IsPrepass)
			{
				if(expectedtype == VM::EpochType_Identifier)
					EmitterStack.top()->PushStringLiteral(handle);
				else
					EmitterStack.top()->PushVariableValue(handle);
			}
			ctparam.StringPayload = Strings.top();
			ctparam.Payload.StringHandleValue = handle;
			Strings.pop();
		}
		break;

	case ITEMTYPE_STRINGLITERAL:
		if(!IsPrepass)
			EmitterStack.top()->PushStringLiteral(StringLiterals.top());
		ctparam.Payload.StringHandleValue = StringLiterals.top();
		StringLiterals.pop();
		break;

	case ITEMTYPE_INTEGERLITERAL:
		if(!IsPrepass)
			EmitterStack.top()->PushIntegerLiteral(IntegerLiterals.top());
		ctparam.Payload.IntegerValue = IntegerLiterals.top();
		IntegerLiterals.pop();
		break;

	default:
		throw NotImplementedException("The parser stack contains something we aren't ready to handle in CompilationSemantics::ValidateAndPushParam");
	}

	PushedItemTypes.pop();
	if(!IsPrepass)
		CompileTimeParameters.top().push_back(ctparam);
}

//
// Helper routine for type-checking parameters
//
void CompilationSemantics::CheckParameterValidity(VM::EpochTypeID expectedtype)
{
	bool valid = true;

	if(PushedItemTypes.empty())
		throw ParameterException("Expected at least one parameter here, but none have been provided");

	switch(PushedItemTypes.top())
	{
	case ITEMTYPE_STATEMENT:
		if(StatementTypes.empty())
			valid = false;
		else if(StatementTypes.top() != expectedtype)
		{
			valid = false;
			StatementTypes.pop();
		}
		break;

	case ITEMTYPE_STRING:
		if(expectedtype != VM::EpochType_Identifier)
		{
			if(GetLexicalScopeDescription(LexicalScopeStack.top()).HasVariable(Strings.top()))
				valid = (GetLexicalScopeDescription(LexicalScopeStack.top()).GetVariableTypeByID(Session.StringPool.Pool(Strings.top())) == expectedtype);
			else
				valid = false;
		}
		break;

	case ITEMTYPE_STRINGLITERAL:
		if(expectedtype != VM::EpochType_String)
			valid = false;
		break;
	
	case ITEMTYPE_INTEGERLITERAL:
		if(expectedtype != VM::EpochType_Integer)
			valid = false;
		break;

	default:
		throw FatalException("Unrecognized ItemType value in LastPushedItemType, parser is probably broken");
	}

	if(!valid)
		Throw(TypeMismatchException("Wrong parameter type"));
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
		for(std::map<StringHandle, std::wstring>::const_iterator iter = Session.StringPool.GetInternalPool().begin(); iter != Session.StringPool.GetInternalPool().end(); ++iter)
			EmitterStack.top()->PoolString(iter->first, iter->second);

		for(std::map<StringHandle, ScopeDescription>::const_iterator iter = LexicalScopeDescriptions.begin(); iter != LexicalScopeDescriptions.end(); ++iter)
		{
			EmitterStack.top()->DefineLexicalScope(iter->first, iter->second.GetVariableCount());
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
	std::map<StringHandle, ScopeDescription>::iterator iter = LexicalScopeDescriptions.find(scopename);
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
	std::set<StringHandle>& overloadednames = FunctionOverloadNames[originalname];
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
void CompilationSemantics::RemapFunctionToOverload(const std::vector<CompileTimeParameter>& params, std::wstring& out_remappedname, StringHandle& out_remappednamehandle) const
{
	std::map<StringHandle, std::set<StringHandle> >::const_iterator overloadsiter = FunctionOverloadNames.find(out_remappednamehandle);
	if(overloadsiter == FunctionOverloadNames.end())
		return;

	const std::set<StringHandle>& overloadednames = overloadsiter->second;
	for(std::set<StringHandle>::const_iterator iter = overloadednames.begin(); iter != overloadednames.end(); ++iter)
	{
		FunctionSignatureSet::const_iterator signatureiter = Session.FunctionSignatures.find(*iter);
		if(signatureiter == Session.FunctionSignatures.end())
			throw FatalException("Tried to map a function overload to an undefined function signature");

		if(params.size() != signatureiter->second.GetNumParameters())
			continue;

		bool matched = true;
		for(size_t i = 0; i < signatureiter->second.GetNumParameters(); ++i)
		{
			if(params[i].Type != signatureiter->second.GetParameterType(i))
			{
				matched = false;
				break;
			}
		}

		if(matched)
		{
			out_remappednamehandle = *iter;
			out_remappedname = Session.StringPool.GetPooledString(out_remappednamehandle);
			return;
		}
	}

	throw FatalException("Failed to remap function overload internally");
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

	throw NotImplementedException("Cannot map the type name \"" + narrow(type) + "\" to an internal type ID");
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
	|| !PushedItemTypes.empty() || !ReturnsIncludedStatement.empty() || !FunctionReturnTypeNames.empty())
		throw FatalException("Parser leaked a resource");
}
