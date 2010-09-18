//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper for handling semantic actions invoked by the parser
//
// This particular wrapper is designed to pass along the semantic
// actions to a bound compiler session, which will then take care
// of emitting the corresponding bytecode.
//

#pragma once


// Dependencies
#include "Parser/SemanticActionInterface.h"

#include "Utility/StringPool.h"
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IDTypes.h"

#include "Metadata/FunctionSignature.h"
#include "Metadata/ScopeDescription.h"
#include "Metadata/CompileTimeParams.h"

#include "Libraries/Library.h"

#include "Compiler/Session.h"
#include "Compiler/ByteCodeEmitter.h"

#include <stack>


class CompilationSemantics : public SemanticActionInterface
{
// Internal constants
private:
	enum ItemType
	{
		ITEMTYPE_NOTHING,
		ITEMTYPE_STATEMENT,
		ITEMTYPE_STRING,
		ITEMTYPE_STRINGLITERAL,
		ITEMTYPE_INTEGERLITERAL,
	};

// Construction
public:
	CompilationSemantics(ByteCodeEmitter& emitter, CompileSession& session)
		: Emitter(emitter),
		  Session(session),
		  ExpressionDepth(0),
		  LastPushedItemType(ITEMTYPE_NOTHING),
		  IsPrepass(true),
		  CompileTimeHelpers(session.CompileTimeHelpers)
	{ }

// Semantic action implementations
public:
	virtual void SetPrepassMode(bool isprepass)
	{ IsPrepass = isprepass; }

	virtual void StoreString(const std::wstring& strliteral);
	virtual void StoreIntegerLiteral(Integer32 value);
	virtual void StoreStringLiteral(const std::wstring& value);

	virtual void StoreEntityType(Bytecode::EntityTag typetag);
	virtual void StoreEntityCode();

	virtual void StoreInfix(const std::wstring& identifier);
	virtual void CompleteInfix();

	virtual void BeginParameterSet();
	virtual void EndParameterSet();
	virtual void RegisterParameterType(const std::wstring& type);
	virtual void RegisterParameterName(const std::wstring& name);

	virtual void BeginReturnSet();
	virtual void EndReturnSet();
	virtual void RegisterReturnType(const std::wstring& type);
	virtual void RegisterReturnName(const std::wstring& name);
	virtual void RegisterReturnValue();

	virtual void BeginStatement(const std::wstring& statementname);
	virtual void BeginStatementParams();
	virtual void ValidateStatementParam();
	virtual void CompleteStatement();

	virtual void BeginAssignment();
	virtual void CompleteAssignment();

	virtual void Finalize();

	virtual void EmitPendingCode();

// Internal helpers
private:
	void CheckParameterValidity(VM::EpochTypeID expectedtype);

	void AddLexicalScope(StringHandle scopename);
	ScopeDescription& GetLexicalScopeDescription(StringHandle scopename);

	void ValidateAndPushParam(unsigned paramindex);

	VM::EpochTypeID LookupTypeName(const std::wstring& name) const;

// Internal tracking
private:
	bool IsPrepass;

	ByteCodeEmitter& Emitter;
	CompileSession& Session;
	FunctionCompileHelperTable& CompileTimeHelpers;

	std::stack<std::wstring> Strings;
	std::stack<Bytecode::EntityTag> EntityTypeTags;

	std::stack<Integer32> IntegerLiterals;
	std::stack<StringHandle> StringLiterals;

	std::stack<FunctionSignature> FunctionSignatureStack;
	VM::EpochTypeID ParamType;

	std::stack<std::wstring> StatementNames;
	std::stack<unsigned> StatementParamCount;
	std::stack<VM::EpochTypeID> StatementTypes;

	std::wstring TemporaryString;

	std::stack<StringHandle> LexicalScopeStack;
	std::map<StringHandle, ScopeDescription> LexicalScopeDescriptions;

	unsigned ExpressionDepth;

	ItemType LastPushedItemType;

	std::stack<std::vector<CompileTimeParameter> > CompileTimeParameters;

	std::stack<std::wstring> CurrentEntities;
	std::stack<StringHandle> FunctionReturnVars;

	std::stack<std::vector<Byte> > PendingEmissionBuffers;
	std::stack<ByteCodeEmitter> PendingEmitters;

	std::stack<StringHandle> AssignmentTargets;
};

