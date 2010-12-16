//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Implementation of bytecode serialization subsystem
//

#include "pch.h"

#include "Serialization/Serializer.h"
#include "Serialization/Exceptions.h"

#include "Bytecode/Instructions.h"
#include "Bytecode/EntityTags.h"

#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <fstream>

using namespace Serialization;


//
// Helper class for traversing through a bytecode buffer
//
class BufferTraverser
{
// Construction
public:
	BufferTraverser(const Byte* buffer, size_t size)
		: Buffer(buffer), Size(size), Offset(0)
	{ }

// Read interface
public:
	template <typename T>
	T Read()
	{
		const Byte* byteoffset = Buffer + Offset;
		T ret = *reinterpret_cast<const T*>(byteoffset);
		Offset += sizeof(T);
		return ret;
	}

	VM::EpochTypeID ReadTypeAnnotation()
	{
		return static_cast<VM::EpochTypeID>(Read<Integer32>());
	}

	Bytecode::EntityTag ReadEntityTag()
	{
		return static_cast<Bytecode::EntityTag>(Read<Integer32>());
	}
	
	std::wstring ReadTerminatedString()
	{
		std::wstring ret;
		wchar_t c;
		while((c = Read<wchar_t>()) != NULL)
			ret += c;
		return ret;
	}

// State interface
public:
	bool EndOfBuffer() const
	{
		return Offset >= Size;
	}

// Public tracking
public:
	size_t Offset;

// Internal tracking
private:
	const Byte* Buffer;
	const size_t Size;
};


//
// Construct and initialize a serialization wrapper
//
Serializer::Serializer(const DLLAccess::CompilerAccess& compileraccess, DLLAccess::CompiledByteCodeHandle bytecodehandle)
	: CompilerAccess(compileraccess),
	  ByteCodeHandle(bytecodehandle)
{
}


//
// The workhorse function: write the bound bytecode stream to a given file
//
void Serializer::Write(const std::wstring& filename) const
{
	const void* bytecoderaw = CompilerAccess.GetByteCode(ByteCodeHandle);
	const Byte* bytecodebytes = reinterpret_cast<const Byte*>(bytecoderaw);
	size_t size = CompilerAccess.GetByteCodeSize(ByteCodeHandle);

	BufferTraverser traverser(bytecodebytes, size);

	size_t indent = 0;

	std::wofstream outfile(filename.c_str());
	while(!traverser.EndOfBuffer())
	{
		outfile.width(sizeof(traverser.Offset) * 2);		// size of offset (e.g. 4 bytes) times 8 bits per byte divided by 4 bits per hex couplet
		outfile.fill(L'0');
		outfile << std::hex << traverser.Offset << L" ";
		outfile << std::dec;


		Byte b = traverser.Read<Byte>();

		if(b == Bytecode::Instructions::EndEntity)
			--indent;

		for(size_t i = 0; i < indent; ++i)
			outfile << L"\t";

		switch(b)
		{
		case Bytecode::Instructions::Halt:
			outfile << L"HALT\n";
			break;

		case Bytecode::Instructions::NoOp:
			outfile << L"NOOP\n";
			break;

		case Bytecode::Instructions::Push:
			{
				VM::EpochTypeID type = traverser.ReadTypeAnnotation();
				switch(type)
				{
				case VM::EpochType_Error:
				case VM::EpochType_Void:
					throw SerializationException("Failed to serialize untyped PUSH operand");
		
				case VM::EpochType_Identifier:
				case VM::EpochType_CustomBase:
					throw SerializationException("Failed to serialize incorrect PUSH operand");

				case VM::EpochType_Integer:
					outfile << L"PUSH_INT " << traverser.Read<Integer32>() << L"\n";
					break;

				case VM::EpochType_Real:
					outfile << L"PUSH_REAL " << traverser.Read<Real32>() << L"\n";
					break;

				case VM::EpochType_Boolean:
					outfile << L"PUSH_BOOL " << traverser.Read<bool>() << L"\n";
					break;

				case VM::EpochType_String:
					outfile << L"PUSH_STR " << traverser.Read<StringHandle>() << L"\n";
					break;

				case VM::EpochType_Buffer:
					outfile << L"PUSH_BUFFER " << traverser.Read<BufferHandle>() << L"\n";
					break;

				default:
					outfile << L"PUSH " << type << L" ";
					outfile << traverser.Read<StructureHandle>() << L"\n";
				}
			}
			break;

		case Bytecode::Instructions::BindRef:
			outfile << L"BINDREF " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::BindMemberRef:
			outfile << L"BINDMEMBERREF " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::Pop:
			{
				VM::EpochTypeID type = traverser.ReadTypeAnnotation();
				outfile << L"POP " << type << L"\n";
			}
			break;

		case Bytecode::Instructions::Read:
			outfile << L"READ " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::ReadRef:
			outfile << L"READREF\n";
			break;

		case Bytecode::Instructions::Assign:
			outfile << L"WRITE\n";
			break;

		case Bytecode::Instructions::Invoke:
			outfile << L"INVOKE " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::InvokeIndirect:
			outfile << L"INVOKE_INDIRECT " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::Return:
			outfile << L"RETURN\n";
			break;

		case Bytecode::Instructions::SetRetVal:
			outfile << L"SETRET " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::BeginEntity:
			{
				outfile << L"ENTITY " << traverser.ReadEntityTag();
				outfile << L" " << traverser.Read<StringHandle>();
				outfile << L"\n";
				++indent;
			}
			break;

		case Bytecode::Instructions::EndEntity:
			outfile << L"ENDENTITY\n";
			break;

		case Bytecode::Instructions::BeginChain:
			outfile << L"CHAIN\n";
			break;

		case Bytecode::Instructions::EndChain:
			outfile << L"ENDCHAIN\n";
			break;

		case Bytecode::Instructions::InvokeMeta:
			outfile << L"META " << traverser.ReadEntityTag() << L"\n";
			break;

		case Bytecode::Instructions::PoolString:
			outfile << L"POOL_STR " << traverser.Read<StringHandle>() << L" ";
			outfile << traverser.ReadTerminatedString() << L"\n";
			break;

		case Bytecode::Instructions::DefineLexicalScope:
			{
				outfile << L"SCOPE " << traverser.Read<StringHandle>() << L" ";
				outfile << traverser.Read<StringHandle>() << L" ";
				size_t count = traverser.Read<size_t>();
				outfile << count << L" ";
				while(count-- > 0)
				{
					outfile << traverser.Read<StringHandle>() << L" ";
					outfile << traverser.ReadTypeAnnotation() << L" ";
					outfile << traverser.Read<Integer32>() << L" ";
					outfile << traverser.Read<bool>() << L" ";
				}
				outfile << L"\n";
			}
			break;

		case Bytecode::Instructions::PatternMatch:
			{
				outfile << L"PATTERN " << traverser.Read<StringHandle>() << L" ";
				size_t count = traverser.Read<size_t>();
				outfile << count << L" ";
				for(size_t i = 0; i < count; ++i)
				{
					VM::EpochTypeID paramtype = traverser.ReadTypeAnnotation();
					outfile << paramtype << L" " ;

					bool needsmatching = (traverser.Read<Byte>() != 0);
					if(needsmatching)
					{
						outfile << L"MATCH ";
						switch(paramtype)
						{
						case VM::EpochType_Integer:
							{
								Integer32 matchvalue = traverser.Read<Integer32>();
								outfile << matchvalue << L" ";
							}
							break;

						default:
							throw NotImplementedException("Pattern matching for this parameter type is not implemented");
						}
					}
					else
						outfile << L"SKIP ";
				}
			}
			outfile << L"\n";
			break;

		case Bytecode::Instructions::AllocStructure:
			outfile << L"ALLOCSTRUCT " << traverser.ReadTypeAnnotation() << L"\n";
			break;

		case Bytecode::Instructions::DefineStructure:
			{
				outfile << L"STRUCT " << traverser.ReadTypeAnnotation();
				size_t nummembers = traverser.Read<size_t>();
				outfile << L" " << nummembers << L" ";
				for(size_t i = 0; i < nummembers; ++i)
				{
					StringHandle identifier = traverser.Read<StringHandle>();
					VM::EpochTypeID type = traverser.ReadTypeAnnotation();
					outfile << identifier << L" " << type << L" ";
				}
				outfile << L"\n";
			}
			break;

		case Bytecode::Instructions::CopyFromStructure:
			{
				outfile << L"SETRETSTRUCT " << traverser.Read<StringHandle>();
				outfile << L" " << traverser.Read<StringHandle>() << L"\n";
			}
			break;

		case Bytecode::Instructions::CopyToStructure:
			{
				outfile << L"SETSTRUCT " << traverser.Read<StringHandle>();
				outfile << L" " << traverser.Read<StringHandle>() << L"\n";
			}
			break;

		case Bytecode::Instructions::CopyBuffer:
			outfile << L"COPY_BUFFER " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::Tag:
			{
				outfile << L"TAG " << traverser.Read<StringHandle>() << L" ";
				size_t numitems = traverser.Read<size_t>();
				outfile << numitems << L" ";
				outfile << traverser.ReadTerminatedString() << L" ";
				for(size_t i = 0; i < numitems; ++i)
					outfile << traverser.ReadTerminatedString() << L" ";
				outfile << L"\n";
			}
			break;

		default:
			throw SerializationException("Failed to serialize unknown opcode");
		}
	}
}