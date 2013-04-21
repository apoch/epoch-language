//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Helper class for generating bytecode sequences
//

#pragma once


// Dependencies
#include "Metadata/ScopeDescription.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/IDTypes.h"

#include "Bytecode/Instructions.h"
#include "Bytecode/EntityTags.h"


// Forward declarations
class FunctionSignature;



//
// Interface for emitting bytecode sequences
//
class BytecodeStreamBase
{
// Modification interface
public:
	virtual void AppendByte(Byte b) = 0;
	virtual void AppendBytes(const Byte* p, size_t size) = 0;

// Retrieval interface
public:
	virtual const Byte* GetPointer() const = 0;
	virtual size_t GetSize() const = 0;
};


//
// Class used for writing bytecode sequences to a std::vector
//
class BytecodeStreamVector : public BytecodeStreamBase
{
// Modification interface
public:
	virtual void AppendByte(Byte b);
	virtual void AppendBytes(const Byte* p, size_t size);

// Retrieval interface
public:
	virtual const Byte* GetPointer() const;
	virtual size_t GetSize() const;

// Internal state
private:
	std::vector<Byte> Bytes;
};

//
// Class used for writing bytecode sequences via an Epoch plugin
//
class BytecodeStreamPlugin : public BytecodeStreamBase
{
// Modification interface
public:
	virtual void AppendByte(Byte b);
	virtual void AppendBytes(const Byte* p, size_t size);

// Retrieval interface
public:
	virtual const Byte* GetPointer() const;
	virtual size_t GetSize() const;
};


//
// Interface for generating bytecode sequences
//
class BytecodeEmitterBase
{
// Functions
public:
	virtual void EnterFunction(StringHandle functionname) = 0;
	virtual void ExitFunction() = 0;

	virtual void SetReturnRegister(size_t variableindex) = 0;

	virtual void EmitFunctionSignature(Metadata::EpochTypeID type, const FunctionSignature& signature) = 0;

// Stack operations
public:
	virtual void PushIntegerLiteral(Integer32 value) = 0;
	virtual void PushInteger16Literal(Integer32 value) = 0;
	virtual void PushStringLiteral(StringHandle handle) = 0;
	virtual void PushBooleanLiteral(bool value) = 0;
	virtual void PushRealLiteral(Real32 value) = 0;
	virtual void PushVariableValue(StringHandle variablename, Metadata::EpochTypeID type) = 0;
	virtual void PushVariableValueNoCopy(StringHandle variablename) = 0;
	virtual void PushLocalVariableValue(bool isparam, size_t frames, size_t offset, size_t size) = 0;
	virtual void PushBufferHandle(BufferHandle handle) = 0;
	virtual void PushTypeAnnotation(Metadata::EpochTypeID type) = 0;
	virtual void PushFunctionNameLiteral(StringHandle funcname) = 0;

	virtual void BindReference(size_t frameskip, size_t variableindex) = 0;
	virtual void BindReferenceIndirect() = 0;
	virtual void BindStructureReference(Metadata::EpochTypeID membertype, size_t memberoffset) = 0;
	virtual void BindStructureReferenceByHandle(StringHandle membername) = 0;

	virtual void PopStack(Metadata::EpochTypeID type) = 0;

// Flow control
public:
	virtual void Invoke(StringHandle functionname) = 0;
	virtual void InvokeIndirect(StringHandle varname) = 0;
	virtual void InvokeOffset(StringHandle functionname) = 0;
	virtual void Halt() = 0;

// Entities
public:
	virtual void EnterEntity(Bytecode::EntityTag tag, StringHandle name) = 0;
	virtual void ExitEntity() = 0;

	virtual void BeginChain() = 0;
	virtual void EndChain() = 0;

	virtual void InvokeMetacontrol(Bytecode::EntityTag tag) = 0;

// Lexical scope metadata
public:
	virtual void DefineLexicalScope(StringHandle name, StringHandle parent, size_t variablecount) = 0;
	virtual void LexicalScopeEntry(StringHandle varname, Metadata::EpochTypeID vartype, bool isreference, VariableOrigin origin) = 0;

// Pattern matching
public:
	virtual void EnterPatternResolver(StringHandle functionname) = 0;
	virtual void ExitPatternResolver() = 0;

	virtual void ResolvePattern(StringHandle dispatchfunction, const FunctionSignature& signature) = 0;

// Type resolution
public:
	virtual void EnterTypeResolver(StringHandle resolvername) = 0;
	virtual void ExitTypeResolver() = 0;

	virtual void ResolveTypes(StringHandle dispatchfunction, const FunctionSignature& signature) = 0;

// Structures
public:
	virtual void AllocateStructure(Metadata::EpochTypeID descriptiontype) = 0;
	virtual void DefineStructure(Metadata::EpochTypeID type, size_t nummembers) = 0;
	virtual void StructureMember(StringHandle identifier, Metadata::EpochTypeID type) = 0;
	virtual void CopyFromStructure(StringHandle structurevariable, StringHandle membervariable) = 0;
	virtual void AssignStructure(StringHandle structurevariable, StringHandle membername) = 0;
	virtual void CopyStructure() = 0;

// Buffers
public:
	virtual void CopyBuffer() = 0;

// Utility instructions
public:
	virtual void AssignVariable() = 0;
	virtual void AssignVariableThroughIdentifier() = 0;
	virtual void AssignSumTypeVariable() = 0;
	virtual void ReadReferenceOntoStack() = 0;
	virtual void ReadReferenceWithTypeAnnotation() = 0;
	
	virtual void TempReferenceFromRegister() = 0;

	virtual void PoolString(StringHandle handle, const std::wstring& literalvalue) = 0;

	virtual void TagData(StringHandle entityname, const std::wstring& tag, const std::vector<std::wstring>& tagdata) = 0;

	virtual void DefineSumType(Metadata::EpochTypeID sumtypeid, const std::set<Metadata::EpochTypeID>& basetypes) = 0;
	virtual void ConstructSumType() = 0;

// Additional helpers for writing to the data stream
public:
	virtual void EmitBuffer(const BytecodeStreamBase& buffer) = 0;

// Internal helpers
private:
	virtual void EmitInstruction(Bytecode::Instruction instruction) = 0;
	virtual void EmitTerminatedString(const std::wstring& value) = 0;

	virtual void EmitTypeAnnotation(Metadata::EpochTypeID type) = 0;
	virtual void EmitEntityTag(Bytecode::EntityTag tag) = 0;

	virtual void EmitRawValue(bool value) = 0;
	virtual void EmitRawValue(Byte value) = 0;
	virtual void EmitRawValue(Integer32 value) = 0;
	virtual void EmitRawValue(Integer16 value) = 0;
	virtual void EmitRawValue(HandleType value) = 0;
	virtual void EmitRawValue(const std::wstring& value) = 0;
	virtual void EmitRawValue(Real32 value) = 0;
};


//
// Wrapper class for generating bytecode sequences
//
// This class is used directly by the code generation pass to create a bytecode
// stream for a program after it has been parsed from source code, and passed
// through validation, type checking, and optimization passes. The resulting
// stream is suitable for serialization either in binary or assembly format,
// or direct execution.
//
class ByteCodeEmitter : public BytecodeEmitterBase
{
// Construction
public:
	explicit ByteCodeEmitter(BytecodeStreamBase& buffer)
		: Buffer(buffer)
	{ }

// Non-copyable
private:
	ByteCodeEmitter(const ByteCodeEmitter& rhs);
	ByteCodeEmitter& operator = (const ByteCodeEmitter& rhs);

// Functions
public:
	void EnterFunction(StringHandle functionname);
	void ExitFunction();

	void SetReturnRegister(size_t variableindex);

	void EmitFunctionSignature(Metadata::EpochTypeID type, const FunctionSignature& signature);

// Stack operations
public:
	void PushIntegerLiteral(Integer32 value);
	void PushInteger16Literal(Integer32 value);
	void PushStringLiteral(StringHandle handle);
	void PushBooleanLiteral(bool value);
	void PushRealLiteral(Real32 value);
	void PushVariableValue(StringHandle variablename, Metadata::EpochTypeID type);
	void PushVariableValueNoCopy(StringHandle variablename);
	void PushLocalVariableValue(bool isparam, size_t frames, size_t offset, size_t size);
	void PushBufferHandle(BufferHandle handle);
	void PushTypeAnnotation(Metadata::EpochTypeID type);
	void PushFunctionNameLiteral(StringHandle funcname);

	void BindReference(size_t frameskip, size_t variableindex);
	void BindReferenceIndirect();
	void BindStructureReference(Metadata::EpochTypeID membertype, size_t memberoffset);
	void BindStructureReferenceByHandle(StringHandle membername);

	void PopStack(Metadata::EpochTypeID type);

// Flow control
public:
	void Invoke(StringHandle functionname);
	void InvokeIndirect(StringHandle varname);
	void InvokeOffset(StringHandle functionname);
	void Halt();

// Entities
public:
	void EnterEntity(Bytecode::EntityTag tag, StringHandle name);
	void ExitEntity();

	void BeginChain();
	void EndChain();

	void InvokeMetacontrol(Bytecode::EntityTag tag);

// Lexical scope metadata
public:
	void DefineLexicalScope(StringHandle name, StringHandle parent, size_t variablecount);
	void LexicalScopeEntry(StringHandle varname, Metadata::EpochTypeID vartype, bool isreference, VariableOrigin origin);

// Pattern matching
public:
	void EnterPatternResolver(StringHandle functionname);
	void ExitPatternResolver();

	void ResolvePattern(StringHandle dispatchfunction, const FunctionSignature& signature);

// Type resolution
public:
	void EnterTypeResolver(StringHandle resolvername);
	void ExitTypeResolver();

	void ResolveTypes(StringHandle dispatchfunction, const FunctionSignature& signature);

// Structures
public:
	void AllocateStructure(Metadata::EpochTypeID descriptiontype);
	void DefineStructure(Metadata::EpochTypeID type, size_t nummembers);
	void StructureMember(StringHandle identifier, Metadata::EpochTypeID type);
	void CopyFromStructure(StringHandle structurevariable, StringHandle membervariable);
	void AssignStructure(StringHandle structurevariable, StringHandle membername);
	void CopyStructure();

// Buffers
public:
	void CopyBuffer();

// Utility instructions
public:
	void AssignVariable();
	void AssignVariableThroughIdentifier();
	void AssignSumTypeVariable();
	void ReadReferenceOntoStack();
	void ReadReferenceWithTypeAnnotation();
	
	void TempReferenceFromRegister();

	void PoolString(StringHandle handle, const std::wstring& literalvalue);

	void TagData(StringHandle entityname, const std::wstring& tag, const std::vector<std::wstring>& tagdata);

	void DefineSumType(Metadata::EpochTypeID sumtypeid, const std::set<Metadata::EpochTypeID>& basetypes);
	void ConstructSumType();

// Additional helpers for writing to the data stream
public:
	void EmitBuffer(const BytecodeStreamBase& buffer);

// Internal helpers
private:
	void EmitInstruction(Bytecode::Instruction instruction);
	void EmitTerminatedString(const std::wstring& value);

	void EmitTypeAnnotation(Metadata::EpochTypeID type);
	void EmitEntityTag(Bytecode::EntityTag tag);

	void EmitRawValue(bool value);
	void EmitRawValue(Byte value);
	void EmitRawValue(Integer32 value);
	void EmitRawValue(Integer16 value);
	void EmitRawValue(HandleType value);
	void EmitRawValue(const std::wstring& value);
	void EmitRawValue(Real32 value);

// Internal tracking
private:
	BytecodeStreamBase& Buffer;
};



//
// Wrapper class for generating bytecode sequences
//
// This class is used by the code generation pass to create a bytecode
// stream via a compiler plugin implemented in Epoch.
//
class BytecodeEmitterPlugin : public BytecodeEmitterBase
{
// Functions
public:
	void EnterFunction(StringHandle functionname);
	void ExitFunction();

	void SetReturnRegister(size_t variableindex);

	void EmitFunctionSignature(Metadata::EpochTypeID type, const FunctionSignature& signature);

// Stack operations
public:
	void PushIntegerLiteral(Integer32 value);
	void PushInteger16Literal(Integer32 value);
	void PushStringLiteral(StringHandle handle);
	void PushBooleanLiteral(bool value);
	void PushRealLiteral(Real32 value);
	void PushVariableValue(StringHandle variablename, Metadata::EpochTypeID type);
	void PushVariableValueNoCopy(StringHandle variablename);
	void PushLocalVariableValue(bool isparam, size_t frames, size_t offset, size_t size);
	void PushBufferHandle(BufferHandle handle);
	void PushTypeAnnotation(Metadata::EpochTypeID type);
	void PushFunctionNameLiteral(StringHandle funcname);

	void BindReference(size_t frameskip, size_t variableindex);
	void BindReferenceIndirect();
	void BindStructureReference(Metadata::EpochTypeID membertype, size_t memberoffset);
	void BindStructureReferenceByHandle(StringHandle membername);

	void PopStack(Metadata::EpochTypeID type);

// Flow control
public:
	void Invoke(StringHandle functionname);
	void InvokeIndirect(StringHandle varname);
	void InvokeOffset(StringHandle functionname);
	void Halt();

// Entities
public:
	void EnterEntity(Bytecode::EntityTag tag, StringHandle name);
	void ExitEntity();

	void BeginChain();
	void EndChain();

	void InvokeMetacontrol(Bytecode::EntityTag tag);

// Lexical scope metadata
public:
	void DefineLexicalScope(StringHandle name, StringHandle parent, size_t variablecount);
	void LexicalScopeEntry(StringHandle varname, Metadata::EpochTypeID vartype, bool isreference, VariableOrigin origin);

// Pattern matching
public:
	void EnterPatternResolver(StringHandle functionname);
	void ExitPatternResolver();

	void ResolvePattern(StringHandle dispatchfunction, const FunctionSignature& signature);

// Type resolution
public:
	void EnterTypeResolver(StringHandle resolvername);
	void ExitTypeResolver();

	void ResolveTypes(StringHandle dispatchfunction, const FunctionSignature& signature);

// Structures
public:
	void AllocateStructure(Metadata::EpochTypeID descriptiontype);
	void DefineStructure(Metadata::EpochTypeID type, size_t nummembers);
	void StructureMember(StringHandle identifier, Metadata::EpochTypeID type);
	void CopyFromStructure(StringHandle structurevariable, StringHandle membervariable);
	void AssignStructure(StringHandle structurevariable, StringHandle membername);
	void CopyStructure();

// Buffers
public:
	void CopyBuffer();

// Utility instructions
public:
	void AssignVariable();
	void AssignVariableThroughIdentifier();
	void AssignSumTypeVariable();
	void ReadReferenceOntoStack();
	void ReadReferenceWithTypeAnnotation();
	
	void TempReferenceFromRegister();

	void PoolString(StringHandle handle, const std::wstring& literalvalue);

	void TagData(StringHandle entityname, const std::wstring& tag, const std::vector<std::wstring>& tagdata);

	void DefineSumType(Metadata::EpochTypeID sumtypeid, const std::set<Metadata::EpochTypeID>& basetypes);
	void ConstructSumType();

// Additional helpers for writing to the data stream
public:
	void EmitBuffer(const BytecodeStreamBase& buffer);

// Internal helpers
private:
	void EmitInstruction(Bytecode::Instruction instruction);
	void EmitTerminatedString(const std::wstring& value);

	void EmitTypeAnnotation(Metadata::EpochTypeID type);
	void EmitEntityTag(Bytecode::EntityTag tag);

	void EmitRawValue(bool value);
	void EmitRawValue(Byte value);
	void EmitRawValue(Integer32 value);
	void EmitRawValue(Integer16 value);
	void EmitRawValue(HandleType value);
	void EmitRawValue(const std::wstring& value);
	void EmitRawValue(Real32 value);

// Visible tracking
public:
	BytecodeStreamPlugin Buffer;
};
