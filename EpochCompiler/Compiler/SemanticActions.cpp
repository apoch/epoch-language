//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper for handling semantic actions invoked by the parser
//

#include "pch.h"

#include "Compiler/SemanticActions.h"
#include "Compiler/ByteCodeEmitter.h"

#include "Metadata/FunctionSignature.h"


void CompilationSemantics::StoreString(const std::wstring& name)
{
	Strings.push(name);
	PushedItemTypes.push(ITEMTYPE_STRING);
}

void CompilationSemantics::StoreIntegerLiteral(Integer32 value)
{
	IntegerLiterals.push(value);
	PushedItemTypes.push(ITEMTYPE_INTEGERLITERAL);
}

void CompilationSemantics::StoreStringLiteral(const std::wstring& value)
{
	StringLiterals.push(Session.StringPool.Pool(value));
	PushedItemTypes.push(ITEMTYPE_STRINGLITERAL);
}

void CompilationSemantics::StoreEntityType(Bytecode::EntityTag typetag)
{
	switch(typetag)
	{
	case Bytecode::EntityTags::Function:
		CurrentEntities.push(Strings.top());
		if(!IsPrepass)
			EmitterStack.top()->EnterFunction(Session.StringPool.Pool(Strings.top()));
		Strings.pop();
		PushedItemTypes.pop();
		break;

	default:
		throw std::exception("Invalid entity type tag");
	}

	EntityTypeTags.push(typetag);
}

void CompilationSemantics::StoreEntityCode()
{
	Bytecode::EntityTag tag = EntityTypeTags.top();
	EntityTypeTags.pop();

	switch(tag)
	{
	case Bytecode::EntityTags::Function:
		if(!IsPrepass)
		{
			FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(Session.StringPool.Pool(CurrentEntities.top()));
			if(iter == Session.FunctionSignatures.end())
				throw std::exception("Failed to locate function being finalized");

			if(iter->second.GetReturnType() != VM::EpochType_Void)
			{
				EmitterStack.top()->SetReturnRegister(FunctionReturnVars.top());
				FunctionReturnVars.pop();
				FunctionReturnTypeNames.pop();
			}

			EmitterStack.top()->ExitFunction();
		}
		break;

	default:
		throw std::exception("Invalid entity type tag");
	}

	CurrentEntities.pop();
	LexicalScopeStack.pop();
}


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

void CompilationSemantics::CompleteInfix()
{
	std::wstring infixstatementname = StatementNames.top();
	StringHandle infixstatementnamehandle = Session.StringPool.Pool(infixstatementname);
	unsigned infixparamcount = StatementParamCount.top();

	StatementNames.pop();
	StatementParamCount.pop();
	PushedItemTypes.push(ITEMTYPE_STATEMENT);

	if(!IsPrepass)
	{
		if(!StatementNames.empty())
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
					throw std::exception("Unknown statement, cannot complete parsing");

				StatementTypes.push(iter->second.GetReturnType());

				if(statementname == L"=")
					CompileTimeParameters.top().push_back(CompileTimeParameter(L"rhs", VM::EpochType_Integer));		// TODO - check type of LHS
				else
				{
					iter = Session.FunctionSignatures.find(Session.StringPool.Pool(statementname));
					if(iter == Session.FunctionSignatures.end())
						throw std::exception("Unknown statement, cannot complete parsing");

					const std::wstring& paramname = iter->second.GetParameterName(StatementParamCount.top());
					VM::EpochTypeID paramtype = iter->second.GetParameterType(StatementParamCount.top());

					CompileTimeParameters.top().push_back(CompileTimeParameter(paramname, paramtype));
				}
			}

			EmitterStack.top()->Invoke(infixstatementnamehandle);
		}
		else
		{
			// We are in a special location such as the initializer of a return value
			EmitterStack.top()->Invoke(infixstatementnamehandle);
			EmitterStack.top()->Invoke(Session.StringPool.Pool(FunctionReturnTypeNames.top()));
		}
	}
}


void CompilationSemantics::BeginParameterSet()
{
	FunctionSignatureStack.push(FunctionSignature());
	AddLexicalScope(Session.StringPool.Pool(Strings.top()));
}

void CompilationSemantics::EndParameterSet()
{
}

void CompilationSemantics::RegisterParameterType(const std::wstring& type)
{
	ParamType = LookupTypeName(type);
}

void CompilationSemantics::RegisterParameterName(const std::wstring& name)
{
	FunctionSignatureStack.top().AddParameter(name, ParamType);
	
	if(!IsPrepass)
	{
		std::map<StringHandle, ScopeDescription>::iterator iter = LexicalScopeDescriptions.find(Session.StringPool.Pool(Strings.top()));
		if(iter == LexicalScopeDescriptions.end())
			throw std::exception("Unregistered lexical scope");
		
		iter->second.AddVariable(name, Session.StringPool.Pool(name), ParamType, VARIABLE_ORIGIN_PARAMETER);
	}
}


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

void CompilationSemantics::EndReturnSet()
{
	if(ReturnsIncludedStatement.top())
	{
		if(!IsPrepass)
			CompileTimeParameters.pop();
	}

	if(IsPrepass)
		Session.FunctionSignatures.insert(std::make_pair(Session.StringPool.Pool(Strings.top()), FunctionSignatureStack.top()));
	else
	{
		EmitPendingCode();
		EmitterStack.pop();
	}

	FunctionSignatureStack.pop();
	ReturnsIncludedStatement.pop();
}

void CompilationSemantics::RegisterReturnType(const std::wstring& type)
{
	if(!IsPrepass)
		FunctionReturnTypeNames.push(type);
	FunctionSignatureStack.top().SetReturnType(LookupTypeName(type));
}

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

void CompilationSemantics::RegisterReturnValue()
{
	if(!IsPrepass)
	{
		PendingEmissionBuffers.push(std::vector<Byte>());
		PendingEmitters.push(ByteCodeEmitter(PendingEmissionBuffers.top()));
	}

	switch(FunctionSignatureStack.top().GetReturnType())
	{
	case VM::EpochType_Integer:
		if(PushedItemTypes.top() == ITEMTYPE_INTEGERLITERAL)
		{
			if(!IsPrepass)
			{
				PendingEmitters.top().PushIntegerLiteral(IntegerLiterals.top());
				PendingEmitters.top().Invoke(Session.StringPool.Pool(L"integer"));
			}
			IntegerLiterals.pop();
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STRING)
		{
			if(!IsPrepass)
			{
				PendingEmitters.top().PushVariableValue(Session.StringPool.Pool(Strings.top()));
				PendingEmitters.top().Invoke(Session.StringPool.Pool(L"integer"));
			}
			Strings.pop();
		}
		else if(PushedItemTypes.top() == ITEMTYPE_STATEMENT)
		{
			ReturnsIncludedStatement.top() = true;
			// TODO - check statement return type for validity
		}
		else
			throw std::exception("Type mismatch");
		break;

	default:
		throw std::exception("Not implemented.");
	}

	PushedItemTypes.pop();
}


void CompilationSemantics::BeginStatement(const std::wstring& statementname)
{
	TemporaryString = statementname;
}

void CompilationSemantics::BeginStatementParams()
{
	++ExpressionDepth;
	StatementNames.push(TemporaryString);
	TemporaryString.clear();
	StatementParamCount.push(0);
	if(!IsPrepass)
		CompileTimeParameters.push(std::vector<CompileTimeParameter>());
}

void CompilationSemantics::ValidateStatementParam()
{
	unsigned paramindex = StatementParamCount.top();
	++StatementParamCount.top();
	ValidateAndPushParam(paramindex);
}

void CompilationSemantics::ValidateAndPushParam(unsigned paramindex)
{
	std::wstring paramname;
	VM::EpochTypeID expectedtype = VM::EpochType_Error;

	if(!IsPrepass)
	{
		if(StatementNames.top() == L"=")
		{
			paramname = L"rhs";
			expectedtype = VM::EpochType_Integer;		// TODO - check type of LHS
		}
		else
		{
			StringHandle statementnamehandle = Session.StringPool.Pool(StatementNames.top());
			FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(statementnamehandle);
			if(iter == Session.FunctionSignatures.end())
				throw std::exception("Unknown statement, cannot validate");

			paramname = iter->second.GetParameterName(paramindex);
			expectedtype = iter->second.GetParameterType(paramindex);
		}

		CheckParameterValidity(expectedtype);
	}

	CompileTimeParameter ctparam(paramname, expectedtype);

	switch(PushedItemTypes.top())
	{
	case ITEMTYPE_STATEMENT:
		break;

	case ITEMTYPE_STRING:
		if(!IsPrepass)
		{
			if(expectedtype == VM::EpochType_Identifier)
				EmitterStack.top()->PushStringLiteral(Session.StringPool.Pool(Strings.top()));
			else
				EmitterStack.top()->PushVariableValue(Session.StringPool.Pool(Strings.top()));
		}
		ctparam.StringPayload = Strings.top();
		ctparam.Payload.StringHandleValue = Session.StringPool.Pool(Strings.top());
		Strings.pop();
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
		throw std::exception("Not implemented");
	}

	PushedItemTypes.pop();
	if(!IsPrepass)
		CompileTimeParameters.top().push_back(ctparam);
}

void CompilationSemantics::CompleteStatement()
{
	--ExpressionDepth;
	std::wstring statementname = StatementNames.top();
	StringHandle statementnamehandle = Session.StringPool.Pool(statementname);

	StatementNames.pop();
	StatementParamCount.pop();
	PushedItemTypes.push(ITEMTYPE_STATEMENT);

	if(!IsPrepass)
	{
		FunctionCompileHelperTable::const_iterator iter = CompileTimeHelpers.find(statementname);
		if(iter != CompileTimeHelpers.end())
			iter->second(GetLexicalScopeDescription(LexicalScopeStack.top()), CompileTimeParameters.top());

		CompileTimeParameters.pop();
		if(!CompileTimeParameters.empty())
		{
			FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(statementnamehandle);
			if(iter == Session.FunctionSignatures.end())
				throw std::exception("Unknown statement, cannot complete parsing");

			StatementTypes.push(iter->second.GetReturnType());

			const std::wstring& paramname = iter->second.GetParameterName(StatementParamCount.top());
			VM::EpochTypeID paramtype = iter->second.GetParameterType(StatementParamCount.top());

			CompileTimeParameters.top().push_back(CompileTimeParameter(paramname, paramtype));
		}

		EmitterStack.top()->Invoke(statementnamehandle);
	}
}

void CompilationSemantics::FinalizeStatement()
{
	if(!IsPrepass)
	{
		StatementTypes.c.clear();
	}
	PushedItemTypes.pop();
}

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


void CompilationSemantics::CheckParameterValidity(VM::EpochTypeID expectedtype)
{
	bool valid = true;

	if(PushedItemTypes.empty())
		throw std::exception("Expected parameter");

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
		throw std::exception("Unrecognized ItemType value in LastPushedItemType, parser is probably broken");
	}

	if(!valid)
		throw std::exception("Wrong parameter type");
}


void CompilationSemantics::AddLexicalScope(StringHandle scopename)
{
	// TODO - check for dupes
	LexicalScopeDescriptions.insert(std::make_pair(scopename, ScopeDescription()));
	LexicalScopeStack.push(scopename);
}

ScopeDescription& CompilationSemantics::GetLexicalScopeDescription(StringHandle scopename)
{
	std::map<StringHandle, ScopeDescription>::iterator iter = LexicalScopeDescriptions.find(scopename);
	if(iter == LexicalScopeDescriptions.end())
		throw std::exception("Unregistered lexical scope");

	return iter->second;
}


VM::EpochTypeID CompilationSemantics::LookupTypeName(const std::wstring& type) const
{
	if(type == L"integer")
		return VM::EpochType_Integer;
	else if(type == L"string")
		return VM::EpochType_String;

	throw std::exception("Unrecognized type");
}


void CompilationSemantics::BeginAssignment()
{
	StatementNames.push(L"=");
	StatementParamCount.push(0);
	AssignmentTargets.push(Session.StringPool.Pool(TemporaryString));
	TemporaryString.clear();
	if(!IsPrepass)
		CompileTimeParameters.push(std::vector<CompileTimeParameter>());
}

void CompilationSemantics::CompleteAssignment()
{
	StatementNames.pop();
	if(!IsPrepass)
	{
		EmitterStack.top()->AssignVariable(AssignmentTargets.top());
		CompileTimeParameters.pop();
		StatementTypes.pop();
	}
	AssignmentTargets.pop();
	StatementParamCount.pop();
}


void CompilationSemantics::SanityCheck() const
{
	if(!Strings.empty() || !EntityTypeTags.empty() || !IntegerLiterals.empty() || !StringLiterals.empty() || !FunctionSignatureStack.empty()
	|| !StatementNames.empty() || !StatementParamCount.empty() || !StatementTypes.empty() || !LexicalScopeStack.empty() || !CompileTimeParameters.empty()
	|| !CurrentEntities.empty() || !FunctionReturnVars.empty() || !PendingEmissionBuffers.empty() || !PendingEmitters.empty() || !AssignmentTargets.empty()
	|| !PushedItemTypes.empty() || !ReturnsIncludedStatement.empty() || !FunctionReturnTypeNames.empty())
		throw std::exception("Parser leaked a resource");
}
