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
#include "Compilation/SemanticActionInterface.h"

#include "Utility/StringPool.h"
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IDTypes.h"

#include "Metadata/FunctionSignature.h"
#include "Metadata/ScopeDescription.h"
#include "Metadata/CompileTimeParams.h"
#include "Metadata/StructureDefinition.h"

#include "Libraries/Library.h"

#include "Compiler/Session.h"
#include "Compiler/ByteCodeEmitter.h"
#include "Compiler/Exceptions.h"

#include <stack>

#include <boost/spirit/include/classic_exceptions.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>


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
		ITEMTYPE_BOOLEANLITERAL,
		ITEMTYPE_REALLITERAL,
	};

// Handy type shortcuts
private:
	typedef std::vector<VM::EpochTypeID> TypeVector;
	typedef std::vector<std::wstring> StringVector;
	typedef std::vector<StringHandle> StringHandles;

	typedef std::pair<StringHandle, size_t> UnaryOperatorAndOffset;
	typedef std::vector<UnaryOperatorAndOffset> UnaryOperatorVector;

	typedef std::multimap<StringHandle, StringHandle> AllOverloadsMap;

	typedef std::map<StringHandle, FunctionSignature> FunctionSignatureMap;

	typedef boost::spirit::classic::position_iterator<const char*> PosIteratorT;
	typedef boost::spirit::classic::parser_error<TypeMismatchException, PosIteratorT> SpiritCompatibleTypeMismatchException;

	typedef std::pair<PosIteratorT, StringHandle> OverloadPositionPair;
	typedef std::list<OverloadPositionPair> OverloadPositionList;

	typedef std::map<VM::EpochTypeID, StructureDefinition> StructureDefinitionMap;
	typedef std::map<StringHandle, VM::EpochTypeID> StructureNameMap;
	typedef std::pair<VM::EpochTypeID, StringHandle> StructureMemberTypeNamePair;
	typedef std::vector<StructureMemberTypeNamePair> StructureMemberList;

// Construction
public:
	CompilationSemantics(ByteCodeEmitter& emitter, CompileSession& session)
		: MasterEmitter(emitter),
		  Session(session),
		  IsPrepass(true),
		  CompileTimeHelpers(session.CompileTimeHelpers),
		  Failed(false),
		  InsideParameterList(false),
		  AnonymousScopeCounter(0),
		  CustomTypeIDCounter(VM::EpochType_CustomBase)
	{
		EmitterStack.push(&MasterEmitter);
	}

	// Semantic action implementations (implementation of SemanticActionInterface)
public:
	virtual void Fail()
	{ Failed = true; }

	virtual bool DidFail() const
	{ return Failed; }

	virtual void SetPrepassMode(bool isprepass);
	virtual bool GetPrepassMode() const
	{ return IsPrepass; }

	virtual void StoreString(const std::wstring& strliteral);
	virtual void StoreIntegerLiteral(Integer32 value);
	virtual void StoreStringLiteral(const std::wstring& value);
	virtual void StoreBooleanLiteral(bool value);
	virtual void StoreRealLiteral(Real32 value);

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

	virtual void RegisterPreOperator(const std::wstring& identifier);
	virtual void RegisterPreOperand(const std::wstring& identifier);
	virtual void RegisterPostOperator(const std::wstring& identifier);
	virtual void RegisterPostOperand(const std::wstring& identifier);

	virtual void StoreUnaryPrefixOperator(const std::wstring& identifier);

	virtual void BeginParameterSet();
	virtual void EndParameterSet();
	virtual void RegisterParameterType(const std::wstring& type);
	virtual void RegisterParameterName(const std::wstring& name);
	virtual void RegisterPatternMatchedParameter();
	virtual void RegisterParameterIsReference();

	virtual void StoreHigherOrderFunctionName(const std::wstring& functionname);
	virtual void BeginHigherOrderFunctionParams();
	virtual void EndHigherOrderFunctionParams();
	virtual void RegisterHigherOrderFunctionParam(const std::wstring& nameoftype);
	virtual void BeginHigherOrderFunctionReturns();
	virtual void EndHigherOrderFunctionReturns();
	virtual void RegisterHigherOrderFunctionReturn(const std::wstring& nameoftype);

	virtual void BeginReturnSet();
	virtual void EndReturnSet();
	virtual void RegisterReturnType(const std::wstring& type);
	virtual void RegisterReturnName(const std::wstring& name);
	virtual void RegisterReturnValue();

	virtual void BeginFunctionTag(const std::wstring& tagname);
	virtual void CompleteFunctionTag();

	virtual void BeginStatement(const std::wstring& statementname);
	virtual void BeginStatementParams();
	virtual void PushStatementParam();
	virtual void CompleteStatement();
	virtual void FinalizeStatement();

	virtual void BeginParenthetical();
	virtual void EndParenthetical();

	virtual void BeginAssignment();
	virtual void BeginOpAssignment(const std::wstring& identifier);
	virtual void CompleteAssignment();

	virtual void StoreStructureName(const std::wstring& identifier);
	virtual const std::wstring& CreateStructureType();
	virtual void StoreStructureMemberType(const std::wstring& type);
	virtual void RegisterStructureMember(const std::wstring& identifier);

	virtual void Finalize();

	virtual void EmitPendingCode();

	virtual void SanityCheck() const;

	virtual void SetParsePosition(const PosIteratorT& iterator)
	{ ParsePosition = iterator; }

	virtual const PosIteratorT& GetParsePosition() const
	{ return ParsePosition; }

	virtual VM::EpochTypeID LookupTypeName(const std::wstring& name) const;

// Internal helpers
private:
	void AddLexicalScope(StringHandle scopename);
	ScopeDescription& GetLexicalScopeDescription(StringHandle scopename);

	void PushParam(const std::wstring& paramname);

	template <typename ExceptionT>
	void Throw(const ExceptionT& exception) const;

	StringHandle AllocateNewOverloadedFunctionName(StringHandle originalname);
	void RemapFunctionToOverload(const CompileTimeParameterVector& params, size_t paramoffset, size_t paramlimit, const TypeVector& possiblereturntypes, std::wstring& out_remappedname, StringHandle& out_remappednamehandle) const;
	void GetAllMatchingOverloads(const CompileTimeParameterVector& params, size_t paramoffset, size_t paramlimit, const TypeVector& possiblereturntypes, const std::wstring& originalname, StringHandle originalnamehandle, StringVector& out_names, StringHandles& out_namehandles) const;

	std::wstring GetPatternMatchResolverName(const std::wstring& originalname) const;

	TypeVector WalkCallChainForExpectedTypes(size_t index) const;

	int GetOperatorPrecedence(StringHandle operatorname) const;

	void EmitInfixOperand(ByteCodeEmitter& emitter, const CompileTimeParameter& ctparam);

	std::wstring AllocateAnonymousScopeName();

	StringHandle FindLexicalScopeName(const ScopeDescription* scope) const;

	Bytecode::EntityTag LookupEntityTag(StringHandle identifier) const;

	void CollapseUnaryOperators();

	void VerifyInfixOperandTypes(StringHandle infixoperator, VM::EpochTypeID op1type, VM::EpochTypeID op2type);

	VM::EpochTypeID GetEffectiveType(const CompileTimeParameter& param) const;

	void CleanAllPushedItems();

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
	std::stack<bool> BooleanLiterals;
	std::stack<Real32> RealLiterals;

	std::stack<FunctionSignature> FunctionSignatureStack;
	std::stack<FunctionSignature> HigherOrderFunctionSignatures;
	VM::EpochTypeID ParamType;

	std::stack<std::wstring> StatementNames;
	std::stack<unsigned> StatementParamCount;
	std::stack<VM::EpochTypeID> StatementTypes;

	std::wstring TemporaryString;

	std::stack<StringHandle> LexicalScopeStack;
	ScopeMap LexicalScopeDescriptions;

	std::stack<ItemType> PushedItemTypes;

	std::stack<CompileTimeParameterVector> CompileTimeParameters;

	std::stack<std::wstring> CurrentEntities;
	std::stack<std::wstring> FunctionReturnTypeNames;
	std::stack<StringHandle> FunctionReturnVars;

	std::stack<ByteBuffer> PendingEmissionBuffers;
	std::stack<ByteCodeEmitter> PendingEmitters;

	std::stack<StringHandle> AssignmentTargets;
	std::stack<bool> ReturnsIncludedStatement;

	PosIteratorT ParsePosition;

	OverloadPositionList OverloadDefinitions;

	bool InsideParameterList;
	bool NeedsPatternResolver;

	FunctionSignatureMap NeededPatternResolvers;
	AllOverloadsMap OriginalFunctionsForPatternResolution;

	std::stack<StringHandles> InfixOperators;

	std::stack<UnaryOperatorVector> UnaryOperators;

	size_t AnonymousScopeCounter;

	VM::EpochTypeID CustomTypeIDCounter;
	StructureDefinitionMap Structures;
	StructureNameMap StructureNames;
	VM::EpochTypeID StructureMemberType;
	StructureMemberList StructureMembers;

	bool ParamIsReference;
};

