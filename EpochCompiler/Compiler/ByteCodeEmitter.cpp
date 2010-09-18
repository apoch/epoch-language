//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Helper class for generating bytecode sequences from semantic actions
//

#include "pch.h"

#include "Compiler/ByteCodeEmitter.h"


void ByteCodeEmitter::EnterFunction(StringHandle functionname)
{
	EmitInstruction(Bytecode::Instructions::BeginEntity);
	EmitEntityTag(Bytecode::EntityTags::Function);
	EmitRawValue(functionname);
}

void ByteCodeEmitter::ExitFunction()
{
	EmitInstruction(Bytecode::Instructions::Return);
	EmitInstruction(Bytecode::Instructions::EndEntity);
}


void ByteCodeEmitter::PushIntegerLiteral(Integer32 value)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(VM::EpochType_Integer);
	EmitRawValue(value);
}

void ByteCodeEmitter::PushStringLiteral(StringHandle handle)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(VM::EpochType_String);
	EmitRawValue(handle);
}


void ByteCodeEmitter::PushVariableValue(StringHandle variablename)
{
	EmitInstruction(Bytecode::Instructions::Read);
	EmitRawValue(variablename);
}


void ByteCodeEmitter::Invoke(StringHandle functionname)
{
	EmitInstruction(Bytecode::Instructions::Invoke);
	EmitRawValue(functionname);
}

void ByteCodeEmitter::Halt()
{
	EmitInstruction(Bytecode::Instructions::Halt);
}


void ByteCodeEmitter::PoolString(StringHandle handle, const std::wstring& literalvalue)
{
	PrependRawValue(literalvalue);
	PrependRawValue(handle);
	PrependInstruction(Bytecode::Instructions::PoolString);
}

void ByteCodeEmitter::DefineLexicalScope(StringHandle name, size_t variablecount)
{
	EmitInstruction(Bytecode::Instructions::DefineLexicalScope);
	EmitRawValue(name);
	EmitRawValue(variablecount);
}

void ByteCodeEmitter::LexicalScopeEntry(StringHandle varname, VM::EpochTypeID vartype, VariableOrigin origin)
{
	EmitRawValue(varname);
	EmitRawValue(vartype);
	EmitRawValue(origin);
}


void ByteCodeEmitter::EmitRawValue(Byte value)
{
	Buffer.push_back(value);
}

void ByteCodeEmitter::EmitRawValue(Integer32 value)
{
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value) & 0xff));
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value >> 8) & 0xff));
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value >> 16) & 0xff));
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value >> 24) & 0xff));
}

void ByteCodeEmitter::EmitRawValue(HandleType value)
{
	EmitRawValue(static_cast<Integer32>(value));
}

void ByteCodeEmitter::EmitRawValue(const std::wstring& value)
{
	std::copy(reinterpret_cast<const Byte*>(value.c_str()), reinterpret_cast<const Byte*>(value.c_str() + value.length() + 1), std::back_inserter(Buffer));
}


void ByteCodeEmitter::PrependBytes(unsigned numbytes, const void* bytes)
{
	const Byte* p = reinterpret_cast<const Byte*>(bytes) + numbytes;
	while((--p) >= reinterpret_cast<const Byte*>(bytes))
		Buffer.insert(Buffer.begin(), *p);
}

void ByteCodeEmitter::PrependRawValue(const std::wstring& value)
{
	wchar_t nullbytes = 0;
	PrependRawValue(nullbytes);
	for(std::wstring::const_reverse_iterator iter = value.rbegin(); iter != value.rend(); ++iter)
		PrependRawValue(*iter);
}



void ByteCodeEmitter::EmitTerminatedString(const std::wstring& value)
{
	EmitRawValue(value);
}

void ByteCodeEmitter::EmitTypeAnnotation(VM::EpochTypeID type)
{
	Integer32 intval = static_cast<Integer32>(type);
	EmitRawValue(intval);
}

void ByteCodeEmitter::PrependTypeAnnotation(VM::EpochTypeID type)
{
	Integer32 intval = static_cast<Integer32>(type);
	PrependRawValue(intval);
}

void ByteCodeEmitter::EmitInstruction(Bytecode::Instruction instruction)
{
	Byte byteval = static_cast<Byte>(instruction);
	EmitRawValue(byteval);
}

void ByteCodeEmitter::PrependInstruction(Bytecode::Instruction instruction)
{
	Byte byteval = static_cast<Byte>(instruction);
	PrependRawValue(byteval);
}

void ByteCodeEmitter::EmitEntityTag(Bytecode::EntityTag tag)
{
	Integer32 intval = static_cast<Integer32>(tag);
	EmitRawValue(intval);
}

