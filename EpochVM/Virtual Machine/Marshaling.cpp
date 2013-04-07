//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Implementation of marshaling for external code invocation and callbacks
//

#include "pch.h"

#include "Virtual Machine/Marshaling.h"
#include "Virtual Machine/VirtualMachine.h"

#include "Utility/Types/IntegerTypes.h"

#include "Utility/DLLPool.h"
#include "Utility/Strings.h"

#include <limits>
#include <list>


namespace
{

	// Prototype of the function which invokes Epoch callbacks
	UInteger32 STDCALL CallbackInvoke(UByte* espsave, VM::ExecutionContext* context, StringHandle callbackfunction);


	//
	// Record for tracking the external library and function
	// that should be invoked by a given [external] tagged
	// Epoch function
	//
	struct DLLInvocationInfo
	{
		std::wstring DLLName;
		std::wstring FunctionName;
	};

	// Map function names to the corresponding invocation record
	std::map<StringHandle, DLLInvocationInfo> DLLInvocationMap;


	//
	// Helper stub for saving stack information when a callback is invoked.
	// This information is used to read parameters off the stack. It is critical
	// that the compiler not emit any prologue or epilogue code for this routine
	// since we need to preserve certain register and pointer values for later
	// retrieval. Note that the callback marshaling stub saves the important
	// context-pointer and function name handle in the eax and edx registers,
	// respectively, on x86 architectures. See the comments on the function
	// MarshalingController::RequestMarshaledCallback for details on how this
	// automatically generated pre-callback code works.
	//
	void __declspec(naked) CallbackEntryPoint()
	{
		__asm mov ecx, esp;

		__asm push edx;
		__asm push eax;
		__asm push ecx;
		__asm call CallbackInvoke;

		__asm ret;
	}


	//
	// Track the mapping between generated callback stubs and
	// the Epoch functions they are intended to invoke
	//
	class MarshalingController
	{
		typedef std::map<StringHandle, void*> MarshalingMapT;
		MarshalingMapT MarshaledCallbackMap;

		// Record describing each allocated stub buffer
		struct StubSpaceRecord
		{
			void* StartOfSpace;
			void* NextAvailableSlot;
		};

		std::vector<StubSpaceRecord> StubSpaceList;

		Threads::CriticalSection MarshalingCriticalSection;

	public:
		//
		// When the controller is destructed, automatically clean up all generated stubs
		//
		~MarshalingController()
		{
			Threads::CriticalSection::Auto mutex(MarshalingCriticalSection);

			MarshaledCallbackMap.clear();

			for(std::vector<StubSpaceRecord>::iterator iter = StubSpaceList.begin(); iter != StubSpaceList.end(); ++iter)
				::VirtualFree(iter->StartOfSpace, 0, MEM_RELEASE);

			StubSpaceList.clear();
		}

	private:
		//
		// Allocate a new chunk of space for generated callback shims
		//
		// WARNING: assumes the critical section is already entered!
		//
		void* AllocNewStubSpace()
		{
			StubSpaceRecord rec;
			rec.StartOfSpace = ::VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			rec.NextAvailableSlot = rec.StartOfSpace;
			StubSpaceList.push_back(rec);
			return rec.NextAvailableSlot;
		}

		//
		// Retrieve the next available slot for generating a new callback shim
		//
		// WARNING: assumes the critical section is already entered!
		//
		void* GetStubSpace()
		{
			if(StubSpaceList.empty())
				return AllocNewStubSpace();

			for(std::vector<StubSpaceRecord>::iterator iter = StubSpaceList.begin(); iter != StubSpaceList.end(); ++iter)
			{
				Byte* start = reinterpret_cast<Byte*>(iter->StartOfSpace);
				Byte* nextslot = reinterpret_cast<Byte*>(iter->NextAvailableSlot);

				if(nextslot < start + 4000)
				{
					nextslot += 64;
					iter->NextAvailableSlot = nextslot;
					return nextslot;
				}
			}

			return AllocNewStubSpace();
		}

	public:
		//
		// Request the marshaling system to provide a callback stub for the
		// given Epoch function. The address of the stub is returned and can
		// be handed off to external code as the callback function.
		//
		// Each generated stub or shim contains a sequence of machine code
		// instructions which perform two central tasks: first, we push some
		// hard-coded parameters onto the stack, then we invoke the callback
		// helper function CallbackEntryPoint defined above. The parameters
		// provide the execution context and Epoch callback function name
		// which should be used to resume execution.
		//
		void* RequestMarshaledCallback(VM::ExecutionContext& context, StringHandle callbackfunction)
		{
			STATIC_ASSERT(sizeof(UINT_PTR) == 4);

			Threads::CriticalSection::Auto mutex(MarshalingCriticalSection);

			// Check if we have already generated a stub for this function
			MarshalingMapT::const_iterator iter = MarshaledCallbackMap.find(callbackfunction);
			if(iter != MarshaledCallbackMap.end())
				return iter->second;

			// Generate a new callback stub
			void* stubspace = GetStubSpace();

			UByte* rawbytes = reinterpret_cast<UByte*>(stubspace);
			UINT_PTR stubaddress = reinterpret_cast<UINT_PTR>(CallbackEntryPoint);
			UINT_PTR callbackaddress = static_cast<UINT_PTR>(callbackfunction);
			UINT_PTR contextaddress = reinterpret_cast<UINT_PTR>(&context);

			// mov ecx, (address)
			rawbytes[0] = 0xB9;
			rawbytes[1] = static_cast<unsigned char>((stubaddress) & 0xFF);
			rawbytes[2] = static_cast<unsigned char>((stubaddress >> 8) & 0xFF);
			rawbytes[3] = static_cast<unsigned char>((stubaddress >> 16) & 0xFF);
			rawbytes[4] = static_cast<unsigned char>((stubaddress >> 24) & 0xFF);

			// mov edx, (function address)
			rawbytes[5] = 0xBA;
			rawbytes[6] = static_cast<unsigned char>((callbackaddress) & 0xFF);
			rawbytes[7] = static_cast<unsigned char>((callbackaddress >> 8) & 0xFF);
			rawbytes[8] = static_cast<unsigned char>((callbackaddress >> 16) & 0xFF);
			rawbytes[9] = static_cast<unsigned char>((callbackaddress >> 24) & 0xFF);

			// mov eax, (context address)
			rawbytes[10] = 0xB8;
			rawbytes[11] = static_cast<unsigned char>((contextaddress) & 0xFF);
			rawbytes[12] = static_cast<unsigned char>((contextaddress >> 8) & 0xFF);
			rawbytes[13] = static_cast<unsigned char>((contextaddress >> 16) & 0xFF);
			rawbytes[14] = static_cast<unsigned char>((contextaddress >> 24) & 0xFF);

			// jmp ecx
			rawbytes[15] = 0xFF;
			rawbytes[16] = 0xE1;

			MarshaledCallbackMap[callbackfunction] = &rawbytes[0];

			return (&rawbytes[0]);
		}
	};

	// Instantiate a controller for marshaling callbacks
	MarshalingController MarshalControl;


	//
	// Actually invoke the Epoch function, taking care to
	// provide the correct parameters from the stack.
	//
	UInteger32 STDCALL CallbackInvoke(UByte* espsave, VM::ExecutionContext* context, StringHandle callbackfunction)
	{
		// TODO - reimplement marshaled callbacks from external C code to Epoch code
		((void)(espsave));
		((void)(context));
		((void)(callbackfunction));

		return 0;
	}


	//
	// Create a special data buffer that contains the marshaled (converted)
	// form of an Epoch structure, suitable for interoperability with standard
	// C/C++ structures. Note that this may modify the memory layout to deal
	// with certain issues like padding and alignment. This function may call
	// itself recursively to deal with nested structures.
	//
	bool MarshalStructureDataIntoBuffer(VM::ExecutionContext& context, StructureHandle structure, const StructureDefinition& definition, Byte* buffer)
	{
		using namespace VM;
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
				*reinterpret_cast<const wchar_t**>(buffer) = context.OwnerVM.GetPooledString(*reinterpret_cast<StringHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j))).c_str();
				buffer += sizeof(const wchar_t*);
				break;

			/*
			case EpochType_Function:
				*reinterpret_cast<void**>(buffer) = MarshalControl.RequestMarshaledCallback(context, *reinterpret_cast<StringHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)));
				buffer += sizeof(void*);
				break;
				*/

			case EpochType_Buffer:
				*reinterpret_cast<void**>(buffer) = context.OwnerVM.GetBuffer(*reinterpret_cast<BufferHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)));
				buffer += sizeof(void*);
				break;

			default:
				if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_TemplateInstance)
				{
					StructureHandle structurehandle = *reinterpret_cast<StructureHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j));
					const StructureDefinition& nesteddefinition = context.OwnerVM.GetStructureDefinition(definition.GetMemberType(j));

					if(!MarshalStructureDataIntoBuffer(context, structurehandle, nesteddefinition, buffer))
						return false;

					buffer += nesteddefinition.GetMarshaledSize();
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
	void MarshalBuffersIntoStructures(VM::ExecutionContext& context, const std::list<MarshaledStructureRecord>& records)
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
void VM::RegisterMarshaledExternalFunction(StringHandle functionname, const std::wstring& dllname, const std::wstring& externalfunctionname)
{
	DLLInvocationInfo info;
	info.DLLName = dllname;
	info.FunctionName = externalfunctionname;
	DLLInvocationMap[functionname] = info;
}


//
// Convert a buffer containing a mutated C/C++ structure back into Epoch format
//
EPOCHVM void VM::MarshalBufferIntoStructureData(VM::ExecutionContext& context, StructureHandle structure, const StructureDefinition& definition, const Byte* buffer)
{
	using namespace VM;
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
					*reinterpret_cast<StringHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) = context.OwnerVM.PoolString(str);
				}
				else
					*reinterpret_cast<Integer32*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j)) = context.OwnerVM.PoolString(L"");
				buffer += sizeof(const wchar_t*);
			}
			break;

			/*
		case EpochType_Function:
			// Function pointers are not marshaled back to Epoch form, currently.
			// This would require an additional interop layer that dynamically
			// relinks Epoch callback functions into their modified targets.
			buffer += sizeof(void*);
			break;
			*/

		case EpochType_Buffer:
			// Buffers are passed as pointers to raw data and if mutated by the
			// external function will not need to be marshaled back manually
			buffer += sizeof(void*);
			break;

		default:
			if(GetTypeFamily(membertype) == EpochTypeFamily_Structure || GetTypeFamily(membertype) == EpochTypeFamily_TemplateInstance)
			{
				StructureHandle structurehandle = *reinterpret_cast<StructureHandle*>(reinterpret_cast<char*>(structure) + definition.GetMemberOffset(j));
				const StructureDefinition& nesteddefinition = context.OwnerVM.GetStructureDefinition(definition.GetMemberType(j));

				MarshalBufferIntoStructureData(context, structurehandle, nesteddefinition, buffer);
				buffer += nesteddefinition.GetMarshaledSize();
			}
			else
			{
				// TODO - halt VM with exception
				return;
			}
		}
	}
}
