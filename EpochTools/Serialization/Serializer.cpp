//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Implementation of bytecode serialization subsystem
//

#include "pch.h"

#include "../../EpochTools/Serialization/Serializer.h"
#include "../../EpochTools/Serialization/Exceptions.h"

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

// Non-copyable
private:
	BufferTraverser(const BufferTraverser& rhs);
	BufferTraverser& operator = (const BufferTraverser& rhs);

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

	Metadata::EpochTypeID ReadTypeAnnotation()
	{
		return static_cast<Metadata::EpochTypeID>(Read<Integer32>());
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
Serializer::Serializer(const void* bytecode, size_t size)
	: BytecodeBuffer(bytecode),
	  BytecodeSize(size)
{
}


//
// The workhorse function: write the bound bytecode stream to a given file
//
void Serializer::Write(const std::wstring& filename) const
{
	const void* bytecoderaw = BytecodeBuffer;
	const Byte* bytecodebytes = reinterpret_cast<const Byte*>(bytecoderaw);
	size_t size = BytecodeSize;

	BufferTraverser traverser(bytecodebytes, size);

	size_t indent = 0;

	std::wofstream outfile(filename.c_str());

	if(!outfile)
		throw FileException("Failed to write to output file");

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
				Metadata::EpochTypeID type = traverser.ReadTypeAnnotation();
				switch(type)
				{
				case Metadata::EpochType_Error:
				case Metadata::EpochType_Void:
					throw SerializationException("Failed to serialize untyped PUSH operand");
		
				case Metadata::EpochType_Identifier:
					throw SerializationException("Failed to serialize incorrect PUSH operand");

				case Metadata::EpochType_Integer:
					outfile << L"PUSH_INT " << traverser.Read<Integer32>() << L"\n";
					break;

				case Metadata::EpochType_Integer16:
					outfile << L"PUSH_INT16 " << traverser.Read<Integer16>() << L"\n";
					break;

				case Metadata::EpochType_Real:
					outfile << L"PUSH_REAL " << traverser.Read<Real32>() << L"\n";
					break;

				case Metadata::EpochType_Boolean:
					outfile << L"PUSH_BOOL " << traverser.Read<bool>() << L"\n";
					break;

				case Metadata::EpochType_String:
					outfile << L"PUSH_STR " << traverser.Read<StringHandle>() << L"\n";
					break;

				case Metadata::EpochType_Buffer:
					outfile << L"PUSH_BUFFER " << traverser.Read<BufferHandle>() << L"\n";
					break;

				case Metadata::EpochTypeFamily_Function:			// Dumb hack.
					outfile << L"PUSH_FUNC " << traverser.Read<StringHandle>() << L"\n";
					break;

				default:
					if(Metadata::IsStructureType(type))
					{
						outfile << L"PUSH " << type << L" ";
						outfile << traverser.Read<StructureHandle>() << L"\n";
					}
					else
					{
						throw SerializationException("Failed to serialize incorrect PUSH operand");
					}
				}
			}
			break;

		case Bytecode::Instructions::BindRef:
			{
				size_t frames = traverser.Read<size_t>();
				size_t index = traverser.Read<size_t>();
				outfile << L"BINDREF " << frames << L" " << index << L"\n";
			}
			break;

		case Bytecode::Instructions::BindMemberRef:
			{
				Metadata::EpochTypeID type = traverser.Read<Metadata::EpochTypeID>();
				size_t offset = traverser.Read<size_t>();
				outfile << L"BINDMEMBERREF " << type << L" " << offset << L"\n";
			}
			break;

		case Bytecode::Instructions::BindMemberByHandle:
			outfile << L"BINDMEMBER " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::Pop:
			{
				size_t bytes = traverser.Read<size_t>();
				outfile << L"POP " << bytes << L"\n";
			}
			break;

		case Bytecode::Instructions::Read:
			outfile << L"READ " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::ReadStack:
			{
				size_t frames = traverser.Read<size_t>();
				size_t offset = traverser.Read<size_t>();
				size_t size = traverser.Read<size_t>();
				outfile << L"READSTACK " << frames << L" " << offset << L" " << size << L"\n";
			}
			break;

		case Bytecode::Instructions::ReadParam:
			{
				size_t frames = traverser.Read<size_t>();
				size_t offset = traverser.Read<size_t>();
				size_t size = traverser.Read<size_t>();
				outfile << L"READPARAM " << frames << L" " << offset << L" " << size << L"\n";
			}
			break;

		case Bytecode::Instructions::ReadRef:
			outfile << L"READREF\n";
			break;

		case Bytecode::Instructions::ReadRefAnnotated:
			outfile << L"READREFANNOTATED\n";
			break;

		case Bytecode::Instructions::Assign:
			outfile << L"WRITE\n";
			break;

		case Bytecode::Instructions::AssignThroughIdentifier:
			outfile << L"WRITETHROUGHID\n";
			break;

		case Bytecode::Instructions::AssignSumType:
			outfile << L"WRITESUMTYPE\n";
			break;

		case Bytecode::Instructions::Invoke:
			outfile << L"INVOKE " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::InvokeOffset:
			{
				StringHandle funcname = traverser.Read<StringHandle>();
				size_t dummyoffset = traverser.Read<size_t>();
				outfile << L"INVOKE_OFFSET " << funcname << L" " << dummyoffset << L"\n";
			}
			break;

		case Bytecode::Instructions::InvokeIndirect:
			outfile << L"INVOKE_INDIRECT " << traverser.Read<StringHandle>() << L"\n";
			break;

		case Bytecode::Instructions::Return:
			outfile << L"RETURN\n";
			break;

		case Bytecode::Instructions::SetRetVal:
			outfile << L"SETRET " << traverser.Read<size_t>() << L"\n";
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
				traverser.Read<size_t>();
				size_t count = traverser.Read<size_t>();
				outfile << count << L" ";
				for(size_t i = 0; i < count; ++i)
				{
					Metadata::EpochTypeID paramtype = traverser.ReadTypeAnnotation();
					outfile << paramtype << L" " ;

					bool needsmatching = (traverser.Read<Byte>() != 0);
					if(needsmatching)
					{
						outfile << L"MATCH ";
						switch(paramtype)
						{
						case Metadata::EpochType_Integer:
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
					Metadata::EpochTypeID type = traverser.ReadTypeAnnotation();
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
			outfile << L"COPY_BUFFER\n";
			break;
			
		case Bytecode::Instructions::CopyStructure:
			outfile << L"COPY_STRUCT\n";
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

		case Bytecode::Instructions::SumTypeDef:
			{
				Metadata::EpochTypeID sumtypeid = traverser.Read<Metadata::EpochTypeID>();
				size_t numentries = traverser.Read<size_t>();

				outfile << L"SUMTYPE " << sumtypeid << L" " << numentries;

				for(size_t i = 0; i < numentries; ++i)
				{
					Metadata::EpochTypeID basetype = traverser.Read<Metadata::EpochTypeID>();
					outfile << L" " << basetype;
				}

				outfile << L"\n";
			}
			break;

		case Bytecode::Instructions::TypeMatch:
			{
				StringHandle dispatchfunction = traverser.Read<StringHandle>();
				traverser.Read<size_t>();
				size_t numparams = traverser.Read<size_t>();

				outfile << L"TYPEMATCH " << dispatchfunction << L" " << numparams;
				for(size_t i = 0; i < numparams; ++i)
				{
					outfile << L" " << traverser.Read<bool>();
					outfile << L" " << traverser.Read<Metadata::EpochTypeID>();
				}

				outfile << L"\n";
			}
			break;

		case Bytecode::Instructions::ConstructSumType:
			outfile << L"CONSTRUCTSUMTYPE\n";
			break;

		case Bytecode::Instructions::TempReferenceFromRegister:
			outfile << L"TYPEFROMREG\n";
			break;

		case Bytecode::Instructions::FuncSignature:
			{
				outfile << L"FUNCSIG ";

				Metadata::EpochTypeID type = traverser.Read<Metadata::EpochTypeID>();
				outfile << type << L" ";

				Metadata::EpochTypeID rettype = traverser.Read<Metadata::EpochTypeID>();
				outfile << rettype << L" ";

				size_t numparams = traverser.Read<size_t>();
				outfile << numparams;

				for(size_t i = 0; i < numparams; ++i)
				{
					outfile << L" " << traverser.Read<Metadata::EpochTypeID>();
					outfile << L" " << traverser.Read<bool>();
				}

				outfile << L"\n";
			}
			break;

		default:
			throw SerializationException("Failed to serialize unknown opcode");
		}
	}
}