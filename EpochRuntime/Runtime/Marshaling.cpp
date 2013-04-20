//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Implementation of marshaling for external code invocation and callbacks
//

#include "pch.h"

#include "Runtime/Marshaling.h"
#include "Runtime/Runtime.h"
#include "Runtime/GlobalContext.h"

#include "Utility/Types/IntegerTypes.h"

#include "Utility/DLLPool.h"
#include "Utility/Strings.h"

#include <limits>
#include <list>


extern "C" void Epoch_Halt();


namespace
{

	// Map function names to the corresponding invocation record
	std::map<StringHandle, Runtime::DLLInvocationInfo> DLLInvocationMap;


	//
	// Create a special data buffer that contains the marshaled (converted)
	// form of an Epoch structure, suitable for interoperability with standard
	// C/C++ structures. Note that this may modify the memory layout to deal
	// with certain issues like padding and alignment. This function may call
	// itself recursively to deal with nested structures.
	//
	bool MarshalStructureDataIntoBuffer(Runtime::ExecutionContext& context, StructureHandle structure, const StructureDefinition& definition, Byte* buffer)
	{
		using namespace Runtime;
		using namespace Metadata;

		for(size_t j = 0; j < definition.GetNumMembers(); ++j)
		{
			EpochTypeID membertype = definition.GetMemberType(j);
			switch(membertype)
			{
			case EpochType_Integer:
				*reinterpret_cast<Integer32*>(buffer) = *reinterpret_cast<Integer32*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j));
				buffer += sizeof(Integer32);
				break;

			case EpochType_Integer16:
				*reinterpret_cast<Integer16*>(buffer) = *reinterpret_cast<Integer16*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j));
				buffer += sizeof(Integer16);
				break;

			case EpochType_Boolean:
				*reinterpret_cast<Integer32*>(buffer) = (*reinterpret_cast<bool*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) ? 1 : 0);
				buffer += sizeof(Integer32);
				break;

			case EpochType_String:
				*reinterpret_cast<const wchar_t**>(buffer) = context.GetPooledString(*reinterpret_cast<StringHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j))).c_str();
				buffer += sizeof(const wchar_t*);
				break;

			case EpochType_Buffer:
				*reinterpret_cast<void**>(buffer) = context.GetBuffer(*reinterpret_cast<BufferHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)));
				buffer += sizeof(void*);
				break;

			default:
				if(Metadata::IsStructureType(membertype))
				{
					StructureHandle structurehandle = *reinterpret_cast<StructureHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j));
					const StructureDefinition& nesteddefinition = context.GetStructureDefinition(definition.GetMemberType(j));

					if(!MarshalStructureDataIntoBuffer(context, structurehandle, nesteddefinition, buffer))
						return false;

					buffer += nesteddefinition.GetMarshaledSize();
				}
				else if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_Function)
				{
					*reinterpret_cast<void**>(buffer) = context.JITCallback(*reinterpret_cast<void**>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)));
					buffer += sizeof(void*);
				}
				else
					return false;
			}
		}

		return true;
	}

	//
	// Record for tracking buffers associated with marshaled data structures
	//
	struct MarshaledStructureRecord
	{
		bool IsReference;
		std::vector<Byte> Buffer;
		StructureHandle Structure;
		const StructureDefinition* Def;

		MarshaledStructureRecord(bool isref, const std::vector<Byte>& buffer, StructureHandle structure, const StructureDefinition& def)
			: IsReference(isref), Buffer(buffer), Structure(structure), Def(&def)
		{ }
	};

	struct MarshaledBooleanRecord
	{
		Integer32 Holder;
		size_t VariableIndex;

		MarshaledBooleanRecord(Integer32 value, size_t index)
			: Holder(value), VariableIndex(index)
		{ }
	};

	//
	// Given a set of structure marshaling records, perform the actual data
	// format conversions for retrieving mutated structures that were passed
	// to the external API by reference from Epoch code.
	//
	void MarshalBuffersIntoStructures(Runtime::ExecutionContext& context, const std::list<MarshaledStructureRecord>& records)
	{
		for(std::list<MarshaledStructureRecord>::const_iterator iter = records.begin(); iter != records.end(); ++iter)
		{
			// Only marshal data for structures passed by reference
			if(!iter->IsReference)
				continue;

			const StructureDefinition& definition = *iter->Def;
			const Byte* buffer = &iter->Buffer[0];
			MarshalBufferIntoStructureData(context, iter->Structure, definition, buffer);
		}
	}
}



//
// Register a tagged external function so we can track what DLL and function to invoke
//
void Runtime::RegisterMarshaledExternalFunction(StringHandle functionname, const std::wstring& dllname, const std::wstring& externalfunctionname, const std::wstring& callingconvention)
{
	DLLInvocationInfo info;
	info.DLLName = dllname;
	info.FunctionName = externalfunctionname;
	info.CallingConvention = callingconvention;
	DLLInvocationMap[functionname] = info;
}

const Runtime::DLLInvocationInfo& Runtime::GetMarshaledExternalFunction(StringHandle alias)
{
	return DLLInvocationMap[alias];
}

bool Runtime::IsMarshaledExternalFunction(StringHandle alias)
{
	return DLLInvocationMap.find(alias) != DLLInvocationMap.end();
}


//
// Convert a buffer containing a mutated C/C++ structure back into Epoch format
//
EPOCHRUNTIME void Runtime::MarshalBufferIntoStructureData(Runtime::ExecutionContext& context, StructureHandle structure, const StructureDefinition& definition, const Byte* buffer)
{
	using namespace Runtime;
	using namespace Metadata;

	for(size_t j = 0; j < definition.GetNumMembers(); ++j)
	{
		EpochTypeID membertype = definition.GetMemberType(j);
		switch(membertype)
		{
		case EpochType_Integer:
			*reinterpret_cast<Integer32*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) = *reinterpret_cast<const Integer32*>(buffer);
			buffer += sizeof(Integer32);
			break;

		case EpochType_Integer16:
			*reinterpret_cast<Integer16*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) = *reinterpret_cast<const Integer16*>(buffer);
			buffer += sizeof(Integer16);
			break;

		case EpochType_Boolean:
			*reinterpret_cast<bool*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) = (*reinterpret_cast<const Integer32*>(buffer)) ? true : false;
			buffer += sizeof(Integer32);
			break;

		case EpochType_String:
			{
				const wchar_t* const* ptr = reinterpret_cast<const wchar_t* const*>(buffer);
				if(ptr && *ptr)
				{
					std::wstring str(*ptr);
					*reinterpret_cast<StringHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) = context.PoolString(str);
				}
				else
					*reinterpret_cast<Integer32*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) = context.PoolString(L"");
				buffer += sizeof(const wchar_t*);
			}
			break;

		case EpochType_Buffer:
			// Buffers are passed as pointers to raw data and if mutated by the
			// external function will not need to be marshaled back manually
			buffer += sizeof(void*);
			break;

		default:
			if(IsStructureType(membertype))
			{
				StructureHandle structurehandle = *reinterpret_cast<StructureHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j));
				const StructureDefinition& nesteddefinition = context.GetStructureDefinition(definition.GetMemberType(j));

				MarshalBufferIntoStructureData(context, structurehandle, nesteddefinition, buffer);
				buffer += nesteddefinition.GetMarshaledSize();
			}
			else if(GetTypeFamily(membertype) == EpochTypeFamily_Function)
			{
				// Function pointers are not marshaled back to Epoch form, currently.
				// This would require an additional interop layer that dynamically
				// relinks Epoch callback functions into their modified targets.
				buffer += sizeof(void*);
			}
			else
			{
				Epoch_Halt();
				return;
			}
		}
	}
}


void Runtime::PopulateWeakLinkages(const std::map<StringHandle, llvm::Function*>& externalfunctions, llvm::ExecutionEngine* ee)
{
	for(std::map<StringHandle, llvm::Function*>::const_iterator iter = externalfunctions.begin(); iter != externalfunctions.end(); ++iter)
	{
        // Now open the DLL and load the corresponding function we wish to invoke
		Marshaling::DLLPool::DLLPoolHandle hdll = Marshaling::TheDLLPool.OpenDLL(DLLInvocationMap[iter->first].DLLName);
        if(!hdll)
		{
			Epoch_Halt();
			return;
        }

        void* address = Marshaling::DLLPool::GetFunction<void*>(hdll, narrow(DLLInvocationMap[iter->first].FunctionName).c_str());
        if(!address)
        {
			Epoch_Halt();
			return;
        }

		ee->updateGlobalMapping(iter->second, address);
	}
}

extern "C" void* MarshalConvertStructure(StructureHandle handle)
{
	ActiveStructure& s = Runtime::GetThreadContext()->FindStructureMetadata(handle);

	Byte* buffer = new Byte[s.Definition.GetMarshaledSize()];

	if(!MarshalStructureDataIntoBuffer(*Runtime::GetThreadContext(), handle, s.Definition, buffer))
		Epoch_Halt();

	return buffer;
}

extern "C" void MarshalFixupStructure(Byte* buffer, StructureHandle target)
{
	ActiveStructure& s = Runtime::GetThreadContext()->FindStructureMetadata(target);
	Runtime::MarshalBufferIntoStructureData(*Runtime::GetThreadContext(), target, s.Definition, buffer);
}

extern "C" void MarshalCleanup(Byte* buffer)
{
	delete[] buffer;
}

extern "C" void* MarshalGenCallback(void* infunc)
{
	return Runtime::GetThreadContext()->JITCallback(infunc);
}


