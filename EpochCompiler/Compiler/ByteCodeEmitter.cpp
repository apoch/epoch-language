//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Helper class for generating bytecode sequences from semantic actions
//

#include "pch.h"

#include "Compiler/ByteCodeEmitter.h"

#include "Metadata/FunctionSignature.h"

#include <sstream>


#define STATIC_ASSERT(expr)			enum { dummy = 1/static_cast<int>(!!(expr)) }


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
// Note that the entity terminator is not executed directly as is the case with most
// entities such as loops or conditionals; instead, it is used primarily by the VM
// as a sort of book-keeping token. In particular it is useful for the Epoch-ASM
// serializer for handling code indentation levels.
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
// finishes execution (for applicable, non-void functions). This implicit push allows
// for easy chaining of expressions and function calls, e.g. where one function's value
// is passed as a parameter to another function without being copied to an intermediate
// variable. The only downside to this approach is the necessity of manually popping the
// stack if/when the return value of a function is ignored, but this is considered a
// minor cost compared to the convenience and efficiency of the register overall.
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
// The stack push instruction accepts a type annotation, which informs the VM
// how many bytes of bytecode to read to retrieve the value that is to be pushed.
// Additionally, this annotation can be used for reference-counting temporary
// values on the stack, which is useful for preventing the garbage collector from
// prematurely collecting objects which are temporarily valid on the stack but
// might become actual garbage soon.
//
void ByteCodeEmitter::PushIntegerLiteral(Integer32 value)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(Metadata::EpochType_Integer);
	EmitRawValue(value);
}

//
// Emit code for pushing a 16-bit integer literal onto the stack
//
// Note that we accept an Integer32 parameter and explicitly convert it to an
// Integer16 within the function. This is done to ensure that the value is not
// truncated silently by the compiler, and that any overflows can be caught and
// generate appropriate errors statically at compile-time.
//
void ByteCodeEmitter::PushInteger16Literal(Integer32 value)
{
	UInteger16 value16 = 0;
	std::wstringstream conversion;
	conversion << value;
	if(!(conversion >> value16))
		throw FatalException("Overflow in integer16 value");

	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(Metadata::EpochType_Integer16);
	EmitRawValue(static_cast<Integer16>(value16));
}

//
// Emit code for pushing a string literal handle onto the stack
//
// String resources are garbage collected and referred to via handles. Strings
// are immutable data, so it is not necessary to provide copy-on-use semantics
// for string variables in the Epoch language. This is in direct contrast with
// the buffer datatype, which is mutable, handle-referenced, and copy-on-use.
//
void ByteCodeEmitter::PushStringLiteral(StringHandle handle)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(Metadata::EpochType_String);
	EmitRawValue(handle);
}

//
// Emit code for pushing a boolean literal onto the stack
//
void ByteCodeEmitter::PushBooleanLiteral(bool value)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(Metadata::EpochType_Boolean);
	EmitRawValue(value);
}

//
// Emit code for pushing a real literal onto the stack
//
void ByteCodeEmitter::PushRealLiteral(Real32 value)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(Metadata::EpochType_Real);
	EmitRawValue(value);
}

//
// Emit code for reading a variable's value and pushing the value onto the stack
//
// Note that some types have special modifications to their semantics for consistency, so it is
// mandatory to know the variable type when emitting this code. For instance, marshaled buffers
// are referenced by handle numbers internally, but copied on accesses in order to create value
// versus handle/reference semantics when used by the programmer. Similar logic helps to ensure
// that structure/object copy constructors are invoked cleanly, and so on. Also worth noting is
// the fact that the emitted code discards the provided type information, because we can always
// emit the correct instructions to perform the required semantics at runtime.
//
void ByteCodeEmitter::PushVariableValue(StringHandle variablename, Metadata::EpochTypeID type)
{
	EmitInstruction(Bytecode::Instructions::Read);
	EmitRawValue(variablename);

	if(type == Metadata::EpochType_Buffer)
		CopyBuffer();
	else if(Metadata::GetTypeFamily(type) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(type) == Metadata::EpochTypeFamily_TemplateInstance)
		CopyStructure();
}

//
// Push the value contained by a variable regardless of its type
//
// Typically if we push a variable containing a reference to a structure or buffer or
// some other handle-controlled resource, we perform a copy operation to provide the
// correct value semantics for the variable rather than implicit reference semantics.
// (This is done for consistency across the language and to make it possible to control
// reference semantics correctly with explicit use of the "ref" keyword.) However, in
// certain cases (such as when passing handles to constructors for initialization) we
// do not wish to deep copy the variable, but instead operate on its handle directly.
// This function allows us to access the handle transparently without copying.
//
void ByteCodeEmitter::PushVariableValueNoCopy(StringHandle variablename)
{
	EmitInstruction(Bytecode::Instructions::Read);
	EmitRawValue(variablename);
}

//
// Emit code for pushing a buffer handle onto the stack
//
// Buffers are garbage collected resources with copy-on-use semantics. They are intended
// primarily for marshaling data and strings to other languages/FFIs. Their use can be
// somewhat finicky due to the semantics involved, and they can easily become a performance
// liability if misused. However, they remain a powerful tool in the Epoch toolbox largely
// because of their many applications in communicating with external C-type APIs.
//
void ByteCodeEmitter::PushBufferHandle(BufferHandle handle)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(Metadata::EpochType_Buffer);
	EmitRawValue(handle);
}

//
// Emit code for binding a reference parameter to a given variable
//
// This is used when passing variables into functions which have ref parameters. Essentially
// we pass on the stack a type annotation and a pointer to the raw storage where the variable's
// actual value can be read and stored; the invoked function resolves this reference as needed,
// essentially performing the required indirection automatically. This instruction is used in
// place of a standard stack push to push the reference binding data onto the stack.
//
void ByteCodeEmitter::BindReference(StringHandle variablename)
{
	PushStringLiteral(variablename);
	EmitInstruction(Bytecode::Instructions::BindRef);
}

//
// Bind a reference, assuming the referred variable identifier is already on the stack
//
void ByteCodeEmitter::BindReferenceIndirect()
{
	EmitInstruction(Bytecode::Instructions::BindRef);
}

//
// Emit code for binding a reference to a given structure member
//
// Assumes that the structure holding the member is already bound
// as a reference prior to this instruction!
//
// This is used when passing a structure member to a function taking a ref parameter, in a
// mechanism very similar to that used by BindReference. However, this is designed to permit
// chained usage, so that nested structures can be supported with one instruction opcode.
//
void ByteCodeEmitter::BindStructureReference(StringHandle membername)
{
	EmitInstruction(Bytecode::Instructions::BindMemberRef);
	EmitRawValue(membername);
}

void ByteCodeEmitter::BindStructureReferenceByHandle(StringHandle membername)
{
	EmitInstruction(Bytecode::Instructions::BindMemberByHandle);
	EmitRawValue(membername);
}

//
// Emit code for popping a given number of bytes off the stack
//
// Note that we do not store the number of bytes to pop; instead, we choose to
// store a type annotation, and the size of that type is determined at runtime
// and used to pop the stack. This is primarily useful for handling structures
// and marshaled datatypes, where the compiler may not know the data size, but
// the VM will.
//
void ByteCodeEmitter::PopStack(Metadata::EpochTypeID type)
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
// The vast majority of VM operation is centered around invoking functions.
// We have deliberately minimized the number of instructions in the VM set,
// meaning that most functionality is implemented via the standard library,
// which uses function invocation to trigger the bulk of the work. Although
// this setup has proven simple to implement, it does leave us with a large
// number of optimization opportunities for mitigating the overhead of many
// function invocations.
//
void ByteCodeEmitter::Invoke(StringHandle functionname)
{
	EmitInstruction(Bytecode::Instructions::Invoke);
	EmitRawValue(functionname);
}

//
// Emit code for invoking a specific function indirectly via a variable
//
// This is used for binding functions into variables and parameters, primarily
// for supporting higher-order functions and lexical closures.
//
void ByteCodeEmitter::InvokeIndirect(StringHandle varname)
{
	EmitInstruction(Bytecode::Instructions::InvokeIndirect);
	EmitRawValue(varname);
}

//
// Emit an instruction which halts execution of the VM
//
// Halting is considered an emergency exit and should be avoided in most
// non-exceptional cases. Note that in the current VM implementation, a
// halt is silent, and will not inform the user as to why the program just
// vanished suddenly.
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
// when a chain is finished executing. The VM preprocesses the locations of these chain
// instructions so that it can cache the offsets for faster execution.
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
// Emit a header for describing a lexical scope
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
void ByteCodeEmitter::LexicalScopeEntry(StringHandle varname, Metadata::EpochTypeID vartype, bool isreference, VariableOrigin origin)
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
// These entities behave similarly to functions, but do not carry explicitly attached lexical
// scopes or code blocks. As such they are designed to appear and disappear on the call stack
// transparently. Pattern resolvers are typically called when it is not possible to determine
// which function should be called statically. Pattern match failures result in exceptions.
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
// Much like a function end, this is intended mainly for a catch-all in case things go
// poorly. In general, it is considered a runtime error for a pattern match to fail. A
// point of similarity to function ends is that the entity end instruction isn't meant
// to be executed, but rather only used for book-keeping.
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
			case Metadata::EpochType_Integer:
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
void ByteCodeEmitter::AllocateStructure(Metadata::EpochTypeID descriptiontype)
{
	EmitInstruction(Bytecode::Instructions::AllocStructure);
	EmitTypeAnnotation(descriptiontype);
}

//
// Emit a meta-data instruction for defining a data structure
//
// Structure definitions are provided to the VM in order to facilitate such things as
// garbage collection (so it is known which fields are references to other resources,
// etc.) and efficient marshaling.
//
void ByteCodeEmitter::DefineStructure(Metadata::EpochTypeID type, size_t nummembers)
{
	EmitInstruction(Bytecode::Instructions::DefineStructure);
	EmitTypeAnnotation(type);
	EmitRawValue(nummembers);
}

//
// Emit meta-data describing a structure member
//
// This presently consists of the identifier handle of the member, and its type.
//
void ByteCodeEmitter::StructureMember(StringHandle identifier, Metadata::EpochTypeID type)
{
	EmitRawValue(identifier);
	EmitTypeAnnotation(type);
}

//
// Emit an instruction to copy a value from a structure into the return value register
//
// We use a special instruction here to facilitate generic member accessor functions via the
// overloaded . (member-access) operator. Structure members are read via these functions,
// which are used to provide dynamic guarantees as required by the type system, where static
// checks cannot suffice. They are also useful for ticking over the garbage collector as well
// as providing correct copy-on-use semantics where necessary.
//
// There is an opportunity for optimization here, where the cost of the accessor function
// invocation can be eliminated by inlining the copy-from-structure instruction and then
// explicitly pushing the register contents on to the stack.
//
// See the discussion above on the return value register for details on its implementation
// and purpose.
//
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
// Since structures are kept on the freestore and not on the stack, it is necessary to facilitate
// writes to structure members via this instruction. This essentially just copies the data from
// the stack-based storage location into the freestore storage location of the member itself.
//
void ByteCodeEmitter::AssignStructure(StringHandle structurevariable, StringHandle membername)
{
	EmitInstruction(Bytecode::Instructions::CopyToStructure);
	EmitRawValue(structurevariable);
	EmitRawValue(membername);
}

//
// Emit an instruction to deep copy a structure
//
// Requires that the handle of the structure be pushed onto the stack prior to the instruction
//
void ByteCodeEmitter::CopyStructure()
{
	EmitInstruction(Bytecode::Instructions::CopyStructure);
}


//-------------------------------------------------------------------------------
// Buffer instructions
//-------------------------------------------------------------------------------

//
// Emit an instruction to clone a buffer
//
// Requires that the handle of the buffer be pushed onto the stack prior to the instruction
//
void ByteCodeEmitter::CopyBuffer()
{
	EmitInstruction(Bytecode::Instructions::CopyBuffer);
}


//-------------------------------------------------------------------------------
// Utility instructions
//-------------------------------------------------------------------------------

//
// Emit an instruction for assigning the contents of the top of the stack into a variable
//
// Note that this is not necessarily the same as just copying from one stack location to another.
// This assignment might transcend local scopes, such as in the case with references, or even
// entire callstacks, as is the case with lexical closures and first-class functions. This op
// assumes that the stack has been prepared with a reference binding (see BindRef) which specifies
// where exactly to write the stack value, and what type of data to copy. Therefore the op itself
// carries no metadata.
//
void ByteCodeEmitter::AssignVariable()
{
	EmitInstruction(Bytecode::Instructions::Assign);
}

//
// Emit an instruction for assigning into a variable indirectly given its identifier
//
// Similar to the AssignVariable() function, this emits an instruction for copying variable
// values (raw values, i.e. including handles sans deep copies) from one location to another.
// However, instead of assuming a reference binding has been established, we assume that an
// indentifier for a variable in the currently active scope is on the stack. This permits the
// assignment of values into newly constructed variables and other indirections.
//
void ByteCodeEmitter::AssignVariableThroughIdentifier()
{
	EmitInstruction(Bytecode::Instructions::AssignThroughIdentifier);
}

void ByteCodeEmitter::AssignSumTypeVariable()
{
	EmitInstruction(Bytecode::Instructions::AssignSumType);
}

//
// Emit an instruction for copying the value of a referenced variable onto the stack
//
// In most circumstances this operation is not necessary; however, it occasionally proves useful,
// as is the case when performing chained assignments such as a=b=c. The stack is assumed to have
// been prepared with a reference binding (see BindRef) so that the target storage and type are
// known. Thus the op carries no metadata.
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
	EmitInstruction(Bytecode::Instructions::PoolString);
	EmitRawValue(handle);
	EmitRawValue(literalvalue);
}


//
// Emit an entity metadata tag to the stream
//
// Metadata tags are used for things like controlling if a function is marshaled to an
// external piece of code, and so on; many are compile-time only and therefore do not
// need to be directly compiled into the final code, but some are useful in the binary
// itself, and will be emitted via this function.
//
void ByteCodeEmitter::TagData(StringHandle entityname, const std::wstring& tag, const std::vector<std::wstring>& tagdata)
{
	EmitInstruction(Bytecode::Instructions::Tag);
	EmitRawValue(entityname);
	EmitRawValue(tagdata.size());
	EmitTerminatedString(tag);
	for(std::vector<std::wstring>::const_iterator iter = tagdata.begin(); iter != tagdata.end(); ++iter)
		EmitTerminatedString(*iter);
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
	STATIC_ASSERT(sizeof(Bytecode::Instruction) <= sizeof(Byte));

	Byte byteval = static_cast<Byte>(instruction);
	EmitRawValue(byteval);
}

//
// Append a null-terminated wide string to the stream
//
// This is provided as distinct from the EmitRawValue overload in order to make the code a
// bit clearer and more readable.
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
void ByteCodeEmitter::EmitTypeAnnotation(Metadata::EpochTypeID type)
{
	STATIC_ASSERT(sizeof(Metadata::EpochTypeID) <= sizeof(Integer32));

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
	STATIC_ASSERT(sizeof(Bytecode::EntityTag) <= sizeof(Integer32));

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
// Append a 16-bit integer value to the stream, in little-endian order
//
void ByteCodeEmitter::EmitRawValue(Integer16 value)
{
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value) & 0xff));
	Buffer.push_back(static_cast<Byte>(static_cast<unsigned char>(value >> 8) & 0xff));
}

//
// Append a 32-bit handle value to the stream, in little-endian order
//
void ByteCodeEmitter::EmitRawValue(HandleType value)
{
	STATIC_ASSERT(sizeof(HandleType) == sizeof(Integer32));

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
	// Cheat and just use the integer version. Note that we're doing a
	// reinterpretation of the raw bits here so there is no need to
	// worry about truncation or rounding errors.
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
void ByteCodeEmitter::PrependTypeAnnotation(Metadata::EpochTypeID type)
{
	STATIC_ASSERT(sizeof(Metadata::EpochTypeID) <= sizeof(Integer32));

	Integer32 intval = static_cast<Integer32>(type);
	PrependRawValue(intval);
}

//
// Prepend a VM instruction to the stream
//
void ByteCodeEmitter::PrependInstruction(Bytecode::Instruction instruction)
{
	STATIC_ASSERT(sizeof(Bytecode::Instruction) <= sizeof(Byte));

	Byte byteval = static_cast<Byte>(instruction);
	PrependRawValue(byteval);
}

void ByteCodeEmitter::PrependEntityTag(Bytecode::EntityTag tag)
{
	STATIC_ASSERT(sizeof(Bytecode::EntityTag) <= sizeof(Integer32));

	Integer32 intval = static_cast<Integer32>(tag);
	PrependRawValue(intval);
}



void ByteCodeEmitter::EnterTypeResolver(StringHandle resolvername)
{
	EmitInstruction(Bytecode::Instructions::BeginEntity);
	EmitEntityTag(Bytecode::EntityTags::TypeResolver);
	EmitRawValue(resolvername);
}

void ByteCodeEmitter::ExitTypeResolver()
{
	// TODO - throw a runtime exception instead of just halting the VM entirely
	Halt();			// Just in case type resolution failed
	EmitInstruction(Bytecode::Instructions::EndEntity);
}

void ByteCodeEmitter::ResolveTypes(StringHandle dispatchfunction, const FunctionSignature& signature)
{
	EmitInstruction(Bytecode::Instructions::TypeMatch);
	EmitRawValue(dispatchfunction);
	EmitRawValue(signature.GetNumParameters());
	for(size_t i = 0; i < signature.GetNumParameters(); ++i)
	{
		EmitRawValue(signature.GetParameter(i).IsReference);
		EmitTypeAnnotation(signature.GetParameter(i).Type);
	}
}

void ByteCodeEmitter::DefineSumType(Metadata::EpochTypeID sumtypeid, const std::set<Metadata::EpochTypeID>& basetypes)
{
	EmitInstruction(Bytecode::Instructions::SumTypeDef);
	EmitTypeAnnotation(sumtypeid);
	EmitRawValue(basetypes.size());
	for(std::set<Metadata::EpochTypeID>::const_iterator iter = basetypes.begin(); iter != basetypes.end(); ++iter)
		EmitTypeAnnotation(*iter);
}

void ByteCodeEmitter::ConstructSumType()
{
	EmitInstruction(Bytecode::Instructions::ConstructSumType);
}

void ByteCodeEmitter::PushTypeAnnotation(Metadata::EpochTypeID type)
{
	EmitInstruction(Bytecode::Instructions::Push);
	EmitTypeAnnotation(Metadata::EpochType_Integer);
	EmitRawValue(type);
}

void ByteCodeEmitter::TypeAnnotationFromRegister()
{
	EmitInstruction(Bytecode::Instructions::TypeFromRegister);
}

