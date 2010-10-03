//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Helper class for generating bytecode sequences from semantic actions
//

#include "pch.h"

#include "Compiler/ByteCodeEmitter.h"

#include "Metadata/FunctionSignature.h"



//-------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------

//
// Emit a function header
//
// Functions are implemented as a specific type of code entity with an attached
// set of parameters, return values, and code block. The function header instructions
// consist of an entity start declaration, the entity tag for functions, and the
// handle of the function's identifier.
//
void ByteCodeEmitter::EnterFunction(StringHandle functionname)
{
	EmitInstruction(Bytecode::Instructions::BeginEntity);
	EmitEntityTag(Bytecode::EntityTags::Function);
	EmitRawValue(functionname);
}

//
// Emit a function termination
//
// Functions always exit with a RETURN instruction to ensure that control flow will
// always progress back to the caller once execution of the attached code block is
// completed. Following the RETURN instruction is an entity terminator declaration.
//
void ByteCodeEmitter::ExitFunction()
{
	EmitInstruction(Bytecode::Instructions::Return);
	EmitInstruction(Bytecode::Instructions::EndEntity);
}

//
// Emit an instruction for setting the contents of the function return value register
//
// As an optimization, the virtual machine contains a special register for holding the
// return value of the last function to exit. The contents of this register are set
// using a special instruction; this function emits that instruction followed by the
// handle of the variable that contains the value to copy into the register. The contents
// of the register are automatically pushed onto the stack by the VM when the function
// finishes execution (for applicable, non-void functions).
//
void ByteCodeEmitter::SetReturnRegister(StringHandle variablename)
{
	EmitInstruction(Bytecode::Instructions::SetRetVal);
	EmitRawValue(variablename);
}


//-------------------------------------------------------------------------------
// Stack operations
//-------------------------------------------------------------------------------

//
// Emit code for pushing a 32-bit integer literal onto the stack
//
void ByteCodeEmitter::PushIntegerLiteral(Integer32 value)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(VM::EpochType_Integer);
	EmitRawValue(value);
}

//
// Emit code for pushing a string literal handle onto the stack
//
void ByteCodeEmitter::PushStringLiteral(StringHandle handle)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(VM::EpochType_String);
	EmitRawValue(handle);
}

//
// Emit code for reading a variable's value and pushing the value onto the stack
//
void ByteCodeEmitter::PushVariableValue(StringHandle variablename)
{
	EmitInstruction(Bytecode::Instructions::Read);
	EmitRawValue(variablename);
}

//
// Emit code for popping a given number of bytes off the stack
//
void ByteCodeEmitter::PopStack(VM::EpochTypeID type)
{
	EmitInstruction(Bytecode::Instructions::Pop);
	EmitTypeAnnotation(type);
}


//-------------------------------------------------------------------------------
// Flow control
//-------------------------------------------------------------------------------

//
// Emit code for invoking a specific function
//
void ByteCodeEmitter::Invoke(StringHandle functionname)
{
	EmitInstruction(Bytecode::Instructions::Invoke);
	EmitRawValue(functionname);
}

//
// Emit an instruction which halts execution of the VM
//
void ByteCodeEmitter::Halt()
{
	EmitInstruction(Bytecode::Instructions::Halt);
}


//-------------------------------------------------------------------------------
// Entities and lexical scopes
//-------------------------------------------------------------------------------

//
// Emit a header describing a lexical scope
//
// Lexical scope metadata consists of a declaration instruction, the handle to the
// scope's internal name (e.g. the name of a function, or a generated name for nested
// scopes within a function, etc.), and the number of data members in the scope.
//
void ByteCodeEmitter::DefineLexicalScope(StringHandle name, size_t variablecount)
{
	EmitInstruction(Bytecode::Instructions::DefineLexicalScope);
	EmitRawValue(name);
	EmitRawValue(variablecount);
}

//
// Emit a description of an entry in a lexical scope
//
// Each descriptor consists of the member's identifier handle, its type annotation, and
// a flag specifying its origin (e.g. local variable, parameter to a function, etc.).
//
void ByteCodeEmitter::LexicalScopeEntry(StringHandle varname, VM::EpochTypeID vartype, VariableOrigin origin)
{
	EmitRawValue(varname);
	EmitRawValue(vartype);
	EmitRawValue(origin);
}


//-------------------------------------------------------------------------------
// Pattern matching
//-------------------------------------------------------------------------------

//
// Enter a special entity which is used for runtime pattern matching on function parameters
//
void ByteCodeEmitter::EnterPatternResolver(StringHandle functionname)
{
	EmitInstruction(Bytecode::Instructions::BeginEntity);
	EmitEntityTag(Bytecode::EntityTags::PatternMatchingResolver);
	EmitRawValue(functionname);
}

//
// Terminate an entity used for pattern matching
//
void ByteCodeEmitter::ExitPatternResolver()
{
	// TODO - throw a runtime exception instead of just halting the VM entirely
	Halt();			// Just in case pattern resolution failed
	EmitInstruction(Bytecode::Instructions::EndEntity);
}

//
// Emit special pattern matching instructions
//
// The pattern match instruction compares the parameter values on the stack to those
// in the given signature, checking for matches. If a match is found, the corresponding
// function overload is invoked. If matching fails, a runtime exception occurs.
//
void ByteCodeEmitter::ResolvePattern(StringHandle dispatchfunction, const FunctionSignature& signature)
{
	EmitInstruction(Bytecode::Instructions::PatternMatch);
	EmitRawValue(dispatchfunction);
	EmitRawValue(signature.GetNumParameters());
	for(size_t i = 0; i < signature.GetNumParameters(); ++i)
	{
		EmitTypeAnnotation(signature.GetParameter(i).Type);

		if(signature.GetParameter(i).Name == L"@@patternmatched")
		{
			// Dump information about this parameter so we can check its value
			EmitRawValue(static_cast<Byte>(1));
			switch(signature.GetParameter(i).Type)
			{
			case VM::EpochType_Integer:
				EmitRawValue(signature.GetParameter(i).Payload.IntegerValue);
				break;

			default:
				throw NotImplementedException("Support for pattern matching function parameters of this type is not implemented");
			}
		}
		else
		{
			// Signal that we can ignore this parameter for function matching purposes
			EmitRawValue(static_cast<Byte>(0));
		}
	}
}


//-------------------------------------------------------------------------------
// Utility instructions
//-------------------------------------------------------------------------------


//
// Emit an instruction for assigning the contents of the top of the stack into a variable
//
// The instruction contains a payload consisting of the handle to the variable whose value
// is to be set by the operation.
//
void ByteCodeEmitter::AssignVariable(StringHandle variablename)
{
	EmitInstruction(Bytecode::Instructions::Assign);
	EmitRawValue(variablename);
}

//
// Emit an instruction for pooling a string identifier or literal
//
// All static strings are pooled using these instructions, which are defined to appear
// at the top of each program. This allows the VM to cache all of the static literals and
// identifiers used by the program for fast access and safe garbage collection.
//
// Pooled strings are specified using a declaration instruction, the handle of the string
// (which is used everywhere else to refer to the string), and the null-terminated value
// of the string itself.
//
void ByteCodeEmitter::PoolString(StringHandle handle, const std::wstring& literalvalue)
{
	// Note that we are using prepend operations here instead of the typical append operations
	// This means that we need to specify the fields backwards to get them to appear in the
	// desired order in the final byte stream.
	PrependRawValue(literalvalue);
	PrependRawValue(handle);
	PrependInstruction(Bytecode::Instructions::PoolString);
}



//-------------------------------------------------------------------------------
// Additional stream writing routines
//-------------------------------------------------------------------------------

//
// Append an arbitrary sequence of bytes to the stream
//
// This is mostly useful if another emitter has cached some instructions to a separate
// stream, and the contents of that stream are to be injected into this one.
//
void ByteCodeEmitter::EmitBuffer(const std::vector<Byte>& buffer)
{
	Buffer.insert(Buffer.end(), buffer.begin(), buffer.end());
}



//-------------------------------------------------------------------------------
// Internal helper routines
//-------------------------------------------------------------------------------

//
// Append a VM instruction to the stream
//
void ByteCodeEmitter::EmitInstruction(Bytecode::Instruction instruction)
{
	if(sizeof(Bytecode::Instruction) > sizeof(Byte))
		throw CompileSettingsException("Truncation of instruction in bytecode emitter");

	Byte byteval = static_cast<Byte>(instruction);
	EmitRawValue(byteval);
}

//
// Append a null-terminated wide string to the stream
//
void ByteCodeEmitter::EmitTerminatedString(const std::wstring& value)
{
	EmitRawValue(value);
}

//
// Append a type annotation constant to the stream
//
// Note that we expand to a 32-bit integer field internally, so that the size of the enum
// as used by the compiler does not adversely affect the size of the data in the stream.
//
void ByteCodeEmitter::EmitTypeAnnotation(VM::EpochTypeID type)
{
	if(sizeof(VM::EpochTypeID) > sizeof(Integer32))
		throw CompileSettingsException("Truncation of type annotation in bytecode emitter");

	Integer32 intval = static_cast<Integer32>(type);
	EmitRawValue(intval);
}

//
// Append an entity tag constant to the stream
//
// Note that we expand to a 32-bit integer field internally, so that the size of the enum
// as used by the compiler does not adversely affect the size of the data in the stream.
//
void ByteCodeEmitter::EmitEntityTag(Bytecode::EntityTag tag)
{
	if(sizeof(Bytecode::EntityTag) > sizeof(Integer32))
		throw CompileSettingsException("Truncation of type annotation in bytecode emitter");

	Integer32 intval = static_cast<Integer32>(tag);
	EmitRawValue(intval);
}

//
// Append a single byte to the stream
//
void ByteCodeEmitter::EmitRawValue(Byte value)
{
	Buffer.push_back(value);
}

//
// Append a 32-bit integer value to the stream, in little-endian order
//
void ByteCodeEmitter::EmitRawValue(Integer32 value)
{
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value) & 0xff));
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value >> 8) & 0xff));
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value >> 16) & 0xff));
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value >> 24) & 0xff));
}

//
// Append a 32-bit handle value to the stream, in little-endian order
//
void ByteCodeEmitter::EmitRawValue(HandleType value)
{
	if(sizeof(HandleType) != sizeof(Integer32))
		throw CompileSettingsException("Mismatched type sizes in bytecode emitter");

	EmitRawValue(static_cast<Integer32>(value));
}

//
// Append a wide string to the stream
//
void ByteCodeEmitter::EmitRawValue(const std::wstring& value)
{
	std::copy(reinterpret_cast<const Byte*>(value.c_str()), reinterpret_cast<const Byte*>(value.c_str() + value.length() + 1), std::back_inserter(Buffer));
}

//
// Prepend a given number of bytes to the stream
//
void ByteCodeEmitter::PrependBytes(unsigned numbytes, const void* bytes)
{
	const Byte* p = reinterpret_cast<const Byte*>(bytes) + numbytes;
	while((--p) >= reinterpret_cast<const Byte*>(bytes))
		Buffer.insert(Buffer.begin(), *p);
}

//
// Prepend a wide string to the stream
//
void ByteCodeEmitter::PrependRawValue(const std::wstring& value)
{
	// Note that we prepend individual characters in reverse order so that the entire string will appear in its original order
	wchar_t nullbytes = 0;
	PrependRawValue(nullbytes);
	for(std::wstring::const_reverse_iterator iter = value.rbegin(); iter != value.rend(); ++iter)
		PrependRawValue(*iter);
}

//
// Prepend a type annotation to the stream
//
// See remarks on ByteCodeEmitter::EmitTypeAnnotation()
//
void ByteCodeEmitter::PrependTypeAnnotation(VM::EpochTypeID type)
{
	if(sizeof(VM::EpochTypeID) > sizeof(Integer32))
		throw CompileSettingsException("Truncation of type annotation in bytecode emitter");

	Integer32 intval = static_cast<Integer32>(type);
	PrependRawValue(intval);
}

//
// Prepend a VM instruction to the stream
//
void ByteCodeEmitter::PrependInstruction(Bytecode::Instruction instruction)
{
	if(sizeof(Bytecode::Instruction) > sizeof(Byte))
		throw CompileSettingsException("Truncation of instruction in bytecode emitter");

	Byte byteval = static_cast<Byte>(instruction);
	PrependRawValue(byteval);
}


