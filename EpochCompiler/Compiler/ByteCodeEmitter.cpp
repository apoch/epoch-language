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
// Emit code for pushing a boolean literal onto the stack
//
void ByteCodeEmitter::PushBooleanLiteral(bool value)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(VM::EpochType_Boolean);
	EmitRawValue(value);
}

//
// Emit code for pushing a real literal onto the stack
//
void ByteCodeEmitter::PushRealLiteral(Real32 value)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(VM::EpochType_Real);
	EmitRawValue(value);
}

//
// Emit code for reading a variable's value and pushing the value onto the stack
//
// Note that some types have special modifications to their semantics for consistency, so it is
// mandatory to know the variable type when emitting this code. For instance, marshaled buffers
// are referenced by handle numbers internally, but copied on accesses in order to create value
// versus handle/reference semantics when used by the programmer. Similar logic helps to ensure
// that structure/object copy constructors are invoked cleanly, and so on.
//
void ByteCodeEmitter::PushVariableValue(StringHandle variablename, VM::EpochTypeID type)
{
	if(type == VM::EpochType_Buffer)
		EmitInstruction(Bytecode::Instructions::CopyBuffer);
	else
		EmitInstruction(Bytecode::Instructions::Read);
	
	EmitRawValue(variablename);
}

//
// Emit code for pushing a buffer handle onto the stack
//
void ByteCodeEmitter::PushBufferHandle(BufferHandle handle)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(VM::EpochType_Buffer);
	EmitRawValue(handle);
}

//
// Emit code for binding a reference parameter to a given variable
//
void ByteCodeEmitter::BindReference(StringHandle variablename)
{
	EmitInstruction(Bytecode::Instructions::BindRef);
	EmitRawValue(variablename);
}

//
// Emit code for binding a reference to a given structure member
//
// Assumes that the structure holding the member is already bound
// as a reference prior to this instruction!
//
void ByteCodeEmitter::BindStructureReference(StringHandle membername)
{
	EmitInstruction(Bytecode::Instructions::BindMemberRef);
	EmitRawValue(membername);
}

//
// Emit code for popping a given number of bytes off the stack
//
// Note that we do not store the number of bytes to pop directly; instead, we
// store a type annotation, and the size of that type is determined at runtime
// and used to pop the stack.
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
// Emit code for invoking a specific function or operator
//
void ByteCodeEmitter::Invoke(StringHandle functionname)
{
	EmitInstruction(Bytecode::Instructions::Invoke);
	EmitRawValue(functionname);
}

//
// Emit code for invoking a specific function indirectly via a variable
//
void ByteCodeEmitter::InvokeIndirect(StringHandle varname)
{
	EmitInstruction(Bytecode::Instructions::InvokeIndirect);
	EmitRawValue(varname);
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
// Emit a generic entity body header
//
// Entities are used for flow control, standalone lexical scopes, compilation directives,
// and so on. Essentially anything with parameters/return values and attached code is an
// entity in the Epoch system. Entering an entity has the effect of activating the lexical
// scope attached to that entity, as well as optionally performing some meta-control code
// to accomplish things like flow control.
//
void ByteCodeEmitter::EnterEntity(Bytecode::EntityTag tag, StringHandle name)
{
	EmitInstruction(Bytecode::Instructions::BeginEntity);
	EmitEntityTag(tag);
	EmitRawValue(name);
}

//
// Emit a generic entity exit
//
// Entity exit instructions are used to know when to clean up the local variable stack and
// destruct any objects that need to go out of scope.
//
void ByteCodeEmitter::ExitEntity()
{
	EmitInstruction(Bytecode::Instructions::EndEntity);
}

//
// Emit an instruction denoting that we are entering an entity chain
//
// Entity chaining is used to allow control flow to optionally pass between one or more
// entities in a chain. This can be used to accomplish if/elseif/else statements, loops,
// exception handlers, switches, and so on. The chain-begin instruction is used to note
// where the instruction pointer should be placed should the meta-control code select to
// repeat the chain, as is the case during loop execution for instance.
//
void ByteCodeEmitter::BeginChain()
{
	EmitInstruction(Bytecode::Instructions::BeginChain);
}

//
// Emit an instruction denoting that we are exiting an entity chain
//
// Entity chain exit instructions are used to know where to jump the instruction pointer
// when a chain is finished executing.
//
void ByteCodeEmitter::EndChain()
{
	EmitInstruction(Bytecode::Instructions::EndChain);
}

//
// Emit an instruction to invoke an entity's meta-control logic
//
// This is sometimes done independently of entering an entity (which is the usual time for
// executing meta-control code); for example, do-while loops invoke the meta-control at the
// end of the loop rather than the beginning, to enforce a minimum of one iteration.
//
void ByteCodeEmitter::InvokeMetacontrol(Bytecode::EntityTag tag)
{
	EmitInstruction(Bytecode::Instructions::InvokeMeta);
	EmitEntityTag(tag);
}


//
// Emit a header describing a lexical scope
//
// Lexical scope metadata consists of a declaration instruction, the handle to the
// scope's internal name (e.g. the name of a function, or a generated name for nested
// scopes within a function, etc.), the handle to the scope's parent name, and the
// number of data members in the scope.
//
void ByteCodeEmitter::DefineLexicalScope(StringHandle name, StringHandle parent, size_t variablecount)
{
	EmitInstruction(Bytecode::Instructions::DefineLexicalScope);
	EmitRawValue(name);
	EmitRawValue(parent);
	EmitRawValue(variablecount);
}

//
// Emit a description of an entry in a lexical scope
//
// Each descriptor consists of the member's identifier handle, its type annotation, and
// a flag specifying its origin (e.g. local variable, parameter to a function, etc.).
//
void ByteCodeEmitter::LexicalScopeEntry(StringHandle varname, VM::EpochTypeID vartype, bool isreference, VariableOrigin origin)
{
	EmitRawValue(varname);
	EmitRawValue(vartype);
	EmitRawValue(origin);
	EmitRawValue(isreference);
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
			EmitRawValue(true);
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
			EmitRawValue(false);
		}
	}
}


//-------------------------------------------------------------------------------
// Structures
//-------------------------------------------------------------------------------

//
// Emit an instruction which allocates a new structure on the freestore
//
void ByteCodeEmitter::AllocateStructure(StringHandle descriptionname)
{
	EmitInstruction(Bytecode::Instructions::AllocStructure);
	EmitRawValue(descriptionname);
}

//
// Emit a meta-data instruction for defining a POD data structure
//
void ByteCodeEmitter::DefineStructure(StringHandle identifier, size_t nummembers)
{
	EmitInstruction(Bytecode::Instructions::DefineStructure);
	EmitRawValue(identifier);
	EmitRawValue(nummembers);
}

//
// Emit meta-data describing a structure member
//
void ByteCodeEmitter::StructureMember(StringHandle identifier, VM::EpochTypeID type)
{
	EmitRawValue(identifier);
	EmitTypeAnnotation(type);
}

//
// Emit an instruction to copy a value from a structure into the return value register
//
void ByteCodeEmitter::CopyFromStructure(StringHandle structurevariable, StringHandle membervariable)
{
	EmitInstruction(Bytecode::Instructions::CopyFromStructure);
	EmitRawValue(structurevariable);
	EmitRawValue(membervariable);
}

//
// Emit an instruction to copy a value from the stack into a structure member
//
void ByteCodeEmitter::AssignStructure(StringHandle structurevariable, StringHandle membername)
{
	EmitInstruction(Bytecode::Instructions::CopyToStructure);
	EmitRawValue(structurevariable);
	EmitRawValue(membername);
}


//-------------------------------------------------------------------------------
// Utility instructions
//-------------------------------------------------------------------------------

//
// Emit an instruction for assigning the contents of the top of the stack into a variable
//
void ByteCodeEmitter::AssignVariable()
{
	EmitInstruction(Bytecode::Instructions::Assign);
}

//
// Emit an instruction for copying the value of a referenced variable onto the stack
//
void ByteCodeEmitter::ReadReferenceOntoStack()
{
	EmitInstruction(Bytecode::Instructions::ReadRef);
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
void ByteCodeEmitter::EmitBuffer(const ByteBuffer& buffer)
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
// Append a boolean to the stream
//
void ByteCodeEmitter::EmitRawValue(bool value)
{
	Buffer.push_back(value ? 1 : 0);
}

//
// Append a real to the stream
//
void ByteCodeEmitter::EmitRawValue(Real32 value)
{
	EmitRawValue(*reinterpret_cast<Integer32*>(&value));
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


