//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Helper class for generating bytecode sequences from semantic actions
//

#pragma once


// Dependencies
#include "Metadata/ScopeDescription.h"

#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IDTypes.h"

#include "Bytecode/Instructions.h"
#include "Bytecode/EntityTags.h"


class ByteCodeEmitter
{
// Construction
public:
	explicit ByteCodeEmitter(std::vector<Byte>& buffer)
		: Buffer(buffer)
	{ }

// Code emission helpers
public:
	void EnterFunction(StringHandle functionname);
	void ExitFunction();

	void SetReturnRegister(StringHandle variablename);

	void PushIntegerLiteral(Integer32 value);
	void PushStringLiteral(StringHandle handle);
	
	void PushVariableValue(StringHandle variablename);
	void AssignVariable(StringHandle variablename);

	void Invoke(StringHandle functionname);

	void Halt();

	void PoolString(StringHandle handle, const std::wstring& literalvalue);

	void DefineLexicalScope(StringHandle name, size_t variablecount);
	void LexicalScopeEntry(StringHandle varname, VM::EpochTypeID vartype, VariableOrigin origin);

	void EmitBuffer(const std::vector<Byte>& buffer);

// Internal helpers
private:
	void EmitRawValue(Byte value);
	void EmitRawValue(Integer32 value);
	void EmitRawValue(HandleType value);
	void EmitRawValue(const std::wstring& value);

	void PrependBytes(unsigned numbytes, const void* bytes);

	template <typename T>
	void PrependRawValue(const T& value)
	{
		PrependBytes(sizeof(T), &value);
	}

	void PrependRawValue(const std::wstring& value);

	void EmitInstruction(Bytecode::Instruction instruction);
	void PrependInstruction(Bytecode::Instruction instruction);

	void EmitEntityTag(Bytecode::EntityTag tag);

	void EmitTypeAnnotation(VM::EpochTypeID type);
	void PrependTypeAnnotation(VM::EpochTypeID type);

	void EmitTerminatedString(const std::wstring& value);

// Internal tracking
private:
	std::vector<Byte>& Buffer;
};

