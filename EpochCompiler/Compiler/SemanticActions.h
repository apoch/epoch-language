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


//
// Wrapper class for handling semantic actions from the Epoch parser and passing on
// the important details to the compiler. This class maintains the vast bulk of the
// state in the parsing process, and really does most of the actual compiling work;
// the emission of actual bytecode is delegated to the ByteCodeEmitter class.
//
class CompilationSemantics : public SemanticActionInterface
{
// Internal constants
private:
	enum ItemType
	{
		ITEMTYPE_STATEMENT,
		ITEMTYPE_STRING,
		ITEMTYPE_STRINGLITERAL,
		ITEMTYPE_INTEGERLITERAL,
	};

// Construction
public:
	CompilationSemantics(ByteCodeEmitter& emitter, CompileSession& session)
		: MasterEmitter(emitter),
		  Session(session),
		  IsPrepass(true),
		  CompileTimeHelpers(session.CompileTimeHelpers)
	{
		EmitterStack.push(&MasterEmitter);
	}

// Semantic action implementations (implementation of SemanticActionInterface)
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
	virtual void FinalizeStatement();

	virtual void BeginAssignment();
	virtual void CompleteAssignment();

	virtual void Finalize();

	virtual void EmitPendingCode();

	virtual void SanityCheck() const;

	virtual void SetParsePosition(const boost::spirit::classic::position_iterator<const char*>& iterator)
	{ ParsePosition = iterator; }

// Internal helpers
private:
	void CheckParameterValidity(VM::EpochTypeID expectedtype);

	void AddLexicalScope(StringHandle scopename);
	ScopeDescription& GetLexicalScopeDescription(StringHandle scopename);

	void ValidateAndPushParam(unsigned paramindex);

	VM::EpochTypeID LookupTypeName(const std::wstring& name) const;

	void Throw(const RecoverableException& exception) const;

// Internal tracking
private:
	bool IsPrepass;

	std::stack<ByteCodeEmitter*> EmitterStack;
	ByteCodeEmitter& MasterEmitter;
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

	std::stack<ItemType> PushedItemTypes;

	std::stack<std::vector<CompileTimeParameter> > CompileTimeParameters;

	std::stack<std::wstring> CurrentEntities;
	std::stack<std::wstring> FunctionReturnTypeNames;
	std::stack<StringHandle> FunctionReturnVars;

	std::stack<std::vector<Byte> > PendingEmissionBuffers;
	std::stack<ByteCodeEmitter> PendingEmitters;

	std::stack<StringHandle> AssignmentTargets;
	std::stack<bool> ReturnsIncludedStatement;

	boost::spirit::classic::position_iterator<const char*> ParsePosition;
};

