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
	LastPushedItemType = ITEMTYPE_STRING;
}

void CompilationSemantics::StoreIntegerLiteral(Integer32 value)
{
	IntegerLiterals.push(value);
	LastPushedItemType = ITEMTYPE_INTEGERLITERAL;
}

void CompilationSemantics::StoreStringLiteral(const std::wstring& value)
{
	LastPushedItemType = ITEMTYPE_STRINGLITERAL;
	StringLiterals.push(Session.StringPool.Pool(value));
}

void CompilationSemantics::StoreEntityType(Bytecode::EntityTag typetag)
{
	switch(typetag)
	{
	case Bytecode::EntityTags::Function:
		if(!IsPrepass)
			Emitter.EnterFunction(Session.StringPool.Pool(Strings.top()));
		Strings.pop();
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
			Emitter.ExitFunction();
		break;

	default:
		throw std::exception("Invalid entity type tag");
	}
}


void CompilationSemantics::StoreInfix(const std::wstring& identifier)
{
	unsigned paramindex = StatementParamCount.top();
	StatementNames.push(identifier);
	StatementParamCount.push(0);
	if(!IsPrepass)
	{
		CompileTimeParameters.push(std::vector<CompileTimeParameter>());
		ValidateAndPushParam(paramindex);
	}
}

void CompilationSemantics::CompleteInfix()
{
	std::wstring infixstatementname = StatementNames.top();
	StringHandle infixstatementnamehandle = Session.StringPool.Pool(infixstatementname);
	unsigned infixparamcount = StatementParamCount.top();

	StatementNames.pop();
	StatementParamCount.pop();
	LastPushedItemType = ITEMTYPE_STATEMENT;

	std::wstring statementname = StatementNames.top();
	StringHandle statementnamehandle = Session.StringPool.Pool(statementname);

	if(!IsPrepass)
	{
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

			iter = Session.FunctionSignatures.find(statementnamehandle);
			if(iter == Session.FunctionSignatures.end())
				throw std::exception("Unknown statement, cannot complete parsing");

			const std::wstring& paramname = iter->second.GetParameterName(StatementParamCount.top());
			VM::EpochTypeID paramtype = iter->second.GetParameterType(StatementParamCount.top());

			CompileTimeParameters.top().push_back(CompileTimeParameter(paramname, paramtype));
		}

		Emitter.Invoke(infixstatementnamehandle);
	}
}


void CompilationSemantics::BeginParameterSet()
{
	FunctionSignatureStack.push(FunctionSignature());
	AddLexicalScope(Session.StringPool.Pool(Strings.top()));
}

void CompilationSemantics::EndParameterSet()
{
	if(IsPrepass)
		Session.FunctionSignatures.insert(std::make_pair(Session.StringPool.Pool(Strings.top()), FunctionSignatureStack.top()));

	FunctionSignatureStack.pop();
}

void CompilationSemantics::RegisterParameterType(const std::wstring& type)
{
	// TODO - improve parameter type detection
	if(type == L"integer")
		ParamType = VM::EpochType_Integer;
	else if(type == L"string")
		ParamType = VM::EpochType_String;
	else
		throw std::exception("Unrecognized type");
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

	if(!IsPrepass)
		ValidateAndPushParam(paramindex);
}

void CompilationSemantics::ValidateAndPushParam(unsigned paramindex)
{
	StringHandle statementnamehandle = Session.StringPool.Pool(StatementNames.top());
	FunctionSignatureSet::const_iterator iter = Session.FunctionSignatures.find(statementnamehandle);
	if(iter == Session.FunctionSignatures.end())
		throw std::exception("Unknown statement, cannot validate");

	VM::EpochTypeID expectedtype = iter->second.GetParameterType(paramindex);
	CheckParameterValidity(expectedtype);
	CompileTimeParameter ctparam(iter->second.GetParameterName(paramindex), expectedtype);

	switch(LastPushedItemType)
	{
	case ITEMTYPE_STATEMENT:
		break;

	case ITEMTYPE_STRING:
		if(expectedtype == VM::EpochType_Identifier)
			Emitter.PushStringLiteral(Session.StringPool.Pool(Strings.top()));
		else
			Emitter.PushVariableValue(Session.StringPool.Pool(Strings.top()));
		ctparam.StringPayload = Strings.top();
		ctparam.Payload.StringHandleValue = Session.StringPool.Pool(Strings.top());
		Strings.pop();
		break;

	case ITEMTYPE_STRINGLITERAL:
		Emitter.PushStringLiteral(StringLiterals.top());
		ctparam.Payload.StringHandleValue = StringLiterals.top();
		StringLiterals.pop();
		break;

	case ITEMTYPE_INTEGERLITERAL:
		Emitter.PushIntegerLiteral(IntegerLiterals.top());
		ctparam.Payload.IntegerValue = IntegerLiterals.top();
		IntegerLiterals.pop();
		break;

	default:
		throw std::exception("Not implemented");
	}

	CompileTimeParameters.top().push_back(ctparam);
}

void CompilationSemantics::CompleteStatement()
{
	--ExpressionDepth;
	std::wstring statementname = StatementNames.top();
	StringHandle statementnamehandle = Session.StringPool.Pool(statementname);

	StatementNames.pop();
	StatementParamCount.pop();
	LastPushedItemType = ITEMTYPE_STATEMENT;

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

		Emitter.Invoke(statementnamehandle);
	}
}


void CompilationSemantics::Finalize()
{
	if(!IsPrepass)
	{
		for(std::map<StringHandle, std::wstring>::const_iterator iter = Session.StringPool.GetInternalPool().begin(); iter != Session.StringPool.GetInternalPool().end(); ++iter)
			Emitter.PoolString(iter->first, iter->second);

		for(std::map<StringHandle, ScopeDescription>::const_iterator iter = LexicalScopeDescriptions.begin(); iter != LexicalScopeDescriptions.end(); ++iter)
		{
			Emitter.DefineLexicalScope(iter->first, iter->second.GetVariableCount());
			for(size_t i = 0; i < iter->second.GetVariableCount(); ++i)
				Emitter.LexicalScopeEntry(Session.StringPool.Pool(iter->second.GetVariableName(i)), iter->second.GetVariableTypeByIndex(i), iter->second.GetVariableOrigin(i));
		}
	}
}



void CompilationSemantics::CheckParameterValidity(VM::EpochTypeID expectedtype)
{
	bool valid = true;

	switch(LastPushedItemType)
	{
	case ITEMTYPE_NOTHING:
		if(expectedtype != VM::EpochType_Error)
			valid = false;
		break;

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

