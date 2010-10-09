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
		  CompileTimeHelpers(session.CompileTimeHelpers),
		  Failed(false),
		  InsideParameterList(false),
		  AnonymousScopeCounter(0)
	{
		EmitterStack.push(&MasterEmitter);
	}

// Check if everything went smooth
public:
	bool DidFail() const
	{ return Failed; }

// Semantic action implementations (implementation of SemanticActionInterface)
public:
	virtual void SetPrepassMode(bool isprepass);

	virtual void StoreString(const std::wstring& strliteral);
	virtual void StoreIntegerLiteral(Integer32 value);
	virtual void StoreStringLiteral(const std::wstring& value);

	virtual void StoreEntityType(Bytecode::EntityTag typetag);
	virtual void StoreEntityType(const std::wstring& identifier);
	virtual void StoreEntityCode();
	virtual void BeginEntityParams();
	virtual void CompleteEntityParams(bool ispostfixcloser);
	virtual void BeginEntityChain();
	virtual void EndEntityChain();
	virtual void StoreEntityPostfix(const std::wstring& identifier);
	virtual void InvokePostfixMetacontrol();

	virtual void StoreInfix(const std::wstring& identifier);
	virtual void PushInfixParam();
	virtual void CompleteInfix();
	virtual void FinalizeInfix();

	virtual void BeginParameterSet();
	virtual void EndParameterSet();
	virtual void RegisterParameterType(const std::wstring& type);
	virtual void RegisterParameterName(const std::wstring& name);
	virtual void RegisterPatternMatchedParameter();

	virtual void BeginReturnSet();
	virtual void EndReturnSet();
	virtual void RegisterReturnType(const std::wstring& type);
	virtual void RegisterReturnName(const std::wstring& name);
	virtual void RegisterReturnValue();

	virtual void BeginStatement(const std::wstring& statementname);
	virtual void BeginStatementParams();
	virtual void PushStatementParam();
	virtual void CompleteStatement();
	virtual void FinalizeStatement();

	virtual void BeginParenthetical();
	virtual void EndParenthetical();

	virtual void BeginAssignment();
	virtual void CompleteAssignment();

	virtual void Finalize();

	virtual void EmitPendingCode();

	virtual void SanityCheck() const;

	virtual void SetParsePosition(const boost::spirit::classic::position_iterator<const char*>& iterator)
	{ ParsePosition = iterator; }

// Internal helpers
private:
	bool CheckParameterValidity(VM::EpochTypeID expectedtype);

	void AddLexicalScope(StringHandle scopename);
	ScopeDescription& GetLexicalScopeDescription(StringHandle scopename);

	void PushParam(const std::wstring& paramname);

	VM::EpochTypeID LookupTypeName(const std::wstring& name) const;

	template <typename ExceptionT>
	void Throw(const ExceptionT& exception) const;

	StringHandle AllocateNewOverloadedFunctionName(StringHandle originalname);
	void RemapFunctionToOverload(const std::vector<CompileTimeParameter>& params, VM::EpochTypeID expectedreturntype, bool allowpartialparamsets, std::wstring& out_remappedname, StringHandle& out_remappednamehandle) const;

	std::wstring GetPatternMatchResolverName(const std::wstring& originalname) const;

	VM::EpochTypeID WalkCallChainForExpectedType(size_t index) const;

	int GetOperatorPrecedence(StringHandle operatorname) const;

	void EmitInfixOperand(ByteCodeEmitter& emitter, const CompileTimeParameter& ctparam);

	std::wstring AllocateAnonymousScopeName();

	StringHandle FindLexicalScopeName(const ScopeDescription* scope) const;

	Bytecode::EntityTag LookupEntityTag(StringHandle identifier) const;

// Internal tracking
private:
	bool IsPrepass;
	bool Failed;

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

	std::list<std::pair<boost::spirit::classic::position_iterator<const char*>, StringHandle> > OverloadDefinitions;

	bool InsideParameterList;
	bool NeedsPatternResolver;

	std::map<StringHandle, FunctionSignature> NeededPatternResolvers;
	std::multimap<StringHandle, StringHandle> OriginalFunctionsForPatternResolution;

	std::stack<std::vector<StringHandle> > InfixOperators;

	size_t AnonymousScopeCounter;
};

