//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Interface for binding a parsing session to a given set of semantic actions
//
// This interface allows us to gently decouple the parser itself from the set
// of code actions that the parsing process invokes. For example, we can bind
// a parser to a compiler, which emits executable bytecode; alternatively, we
// could bind a parser to a static analysis system for validation, and so on.
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Bytecode/EntityTags.h"


class SemanticActionInterface
{
public:
	virtual void SetPrepassMode(bool isprepass) = 0;

	virtual void StoreString(const std::wstring& name) = 0;
	virtual void StoreIntegerLiteral(Integer32 value) = 0;
	virtual void StoreStringLiteral(const std::wstring& value) = 0;

	virtual void StoreEntityType(Bytecode::EntityTag typetag) = 0;
	virtual void StoreEntityCode() = 0;

	virtual void StoreInfix(const std::wstring& identifier) = 0;
	virtual void CompleteInfix() = 0;

	virtual void BeginParameterSet() = 0;
	virtual void EndParameterSet() = 0;
	virtual void RegisterParameterType(const std::wstring& type) = 0;
	virtual void RegisterParameterName(const std::wstring& name) = 0;

	virtual void BeginReturnSet() = 0;
	virtual void EndReturnSet() = 0;
	virtual void RegisterReturnType(const std::wstring& type) = 0;
	virtual void RegisterReturnName(const std::wstring& name) = 0;
	virtual void RegisterReturnValue() = 0;

	virtual void BeginStatement(const std::wstring& statementname) = 0;
	virtual void BeginStatementParams() = 0;
	virtual void ValidateStatementParam() = 0;
	virtual void CompleteStatement() = 0;

	virtual void BeginAssignment() = 0;
	virtual void CompleteAssignment() = 0;

	virtual void Finalize() = 0;

	virtual void EmitPendingCode() = 0;
};

