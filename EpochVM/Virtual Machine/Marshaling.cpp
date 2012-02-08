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

#include "Metadata/ActiveScope.h"

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
		void* espsave;
		__asm mov espsave, esp;

		__asm push edx;
		__asm push eax;
		__asm push espsave;
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
					void* ret = nextslot;
					nextslot += 20;
					iter->NextAvailableSlot = nextslot;
					return ret;
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
			if(sizeof(UINT_PTR) != 4)
				throw CompileSettingsException("Marshaling code is only implemented for 32-bit callbacks");

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
		// We need to increment past the stored value of ESP
		// in order to reach the actual first parameter. This
		// is because of the local variable defined in the
		// callback stub, which uses a stack slot.
		UByte* esp = espsave + sizeof(void*);

		VM::EpochTypeID resulttype = VM::EpochType_Void;
		const ScopeDescription& description = context->OwnerVM.GetScopeDescription(callbackfunction);
		for(size_t i = 0; i < description.GetVariableCount(); ++i)
		{
			if(description.GetVariableOrigin(i) == VARIABLE_ORIGIN_RETURN)
				resulttype = description.GetVariableTypeByIndex(i);
			else if(description.GetVariableOrigin(i) == VARIABLE_ORIGIN_PARAMETER)
			{
				switch(description.GetVariableTypeByIndex(i))
				{
				case VM::EpochType_Boolean:
					context->State.Stack.PushValue<bool>(*reinterpret_cast<Integer32*>(esp) != 0 ? true : false);
					esp += sizeof(Integer32);
					break;

				case VM::EpochType_Integer:
					context->State.Stack.PushValue(*reinterpret_cast<Integer32*>(esp));
					esp += sizeof(Integer32);
					break;

				case VM::EpochType_Real:
					context->State.Stack.PushValue(*reinterpret_cast<Real32*>(esp));
					esp += sizeof(Real32);
					break;

				case VM::EpochType_String:
					{
						StringHandle handle = context->OwnerVM.PoolString(*reinterpret_cast<wchar_t**>(esp));
						context->State.Stack.PushValue(handle);
						context->TickStringGarbageCollector();
						esp += sizeof(wchar_t*);
					}
					break;

				default:
					throw NotImplementedException("Support for this type of function callback parameter is not implemented");
				}
			}
		}
		context->OwnerVM.InvokeFunction(callbackfunction, *context);

		// The Epoch function's return value is used to determine
		// how we leave this shim. We need to clean up local
		// variables from the stack as well as pass on the return
		// value (in the EAX register, as per STDCALL). In
		// order to make sure all the housekeeping is done correctly,
		// we let the compiler generate the return code for us.
		switch(resulttype)
		{
		case VM::EpochType_Void:
			return 0;

		case VM::EpochType_Integer:
			return context->State.Stack.PopValue<Integer32>();

		case VM::EpochType_Boolean:
			return context->State.Stack.PopValue<bool>() ? 1 : 0;

		default:
			throw NotImplementedException("Support for return values of this type from a callback is not yet implemented!");
		}
	}


	//
	// Create a special data buffer that contains the marshaled (converted)
	// form of an Epoch structure, suitable for interoperability with standard
	// C/C++ structures. Note that this may modify the memory layout to deal
	// with certain issues like padding and alignment. This function may call
	// itself recursively to deal with nested structures.
	//
	bool MarshalStructureDataIntoBuffer(VM::ExecutionContext& context, const ActiveStructure& structure, const StructureDefinition& definition, Byte* buffer)
	{
		using namespace VM;

		for(size_t j = 0; j < definition.GetNumMembers(); ++j)
		{
			EpochTypeID membertype = definition.GetMemberType(j);
			switch(membertype)
			{
			case EpochType_Integer:
				*reinterpret_cast<Integer32*>(buffer) = structure.ReadMember<Integer32>(j);
				buffer += sizeof(Integer32);
				break;

			case EpochType_Integer16:
				*reinterpret_cast<Integer16*>(buffer) = structure.ReadMember<Integer16>(j);
				buffer += sizeof(Integer16);
				break;

			case EpochType_Boolean:
				*reinterpret_cast<Integer32*>(buffer) = (structure.ReadMember<bool>(j) ? 1 : 0);
				buffer += sizeof(Integer32);
				break;

			case EpochType_String:
				*reinterpret_cast<const wchar_t**>(buffer) = context.OwnerVM.GetPooledString(structure.ReadMember<StringHandle>(j)).c_str();
				buffer += sizeof(const wchar_t*);
				break;

			case EpochType_Function:
				*reinterpret_cast<void**>(buffer) = MarshalControl.RequestMarshaledCallback(context, structure.ReadMember<StringHandle>(j));
				buffer += sizeof(void*);
				break;

			case EpochType_Buffer:
				*reinterpret_cast<void**>(buffer) = context.OwnerVM.GetBuffer(structure.ReadMember<BufferHandle>(j));
				buffer += sizeof(void*);
				break;

			default:
				if(membertype > EpochType_CustomBase)
				{
					StructureHandle structurehandle = structure.ReadMember<StructureHandle>(j);
					const ActiveStructure& nestedstructure = context.OwnerVM.GetStructure(structurehandle);
					const StructureDefinition& nesteddefinition = context.OwnerVM.GetStructureDefinition(definition.GetMemberType(j));

					if(!MarshalStructureDataIntoBuffer(context, nestedstructure, nesteddefinition, buffer))
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
		ActiveStructure* Structure;

		MarshaledStructureRecord(bool isref, const std::vector<Byte>& buffer, ActiveStructure& structure)
			: IsReference(isref), Buffer(buffer), Structure(&structure)
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

			const StructureDefinition& definition = iter->Structure->Definition;
			const Byte* buffer = &iter->Buffer[0];
			MarshalBufferIntoStructureData(context, *(iter->Structure), definition, buffer);
		}
	}
}

//
// Dispatch execution to an external DLL function
//
void ExternalDispatch(StringHandle functionname, VM::ExecutionContext& context)
{
	using namespace VM;

	// Placeholders for storing off the external function's return value
	Integer32 integerret;
	Integer16 integer16ret;
	const wchar_t* stringret;

	// We use the name of the calling function to map us to the correct external.
	// This is because the calling function itself is the one that was originally
	// tagged as external and associated with the correct DLL/function names.
	StringHandle callingfunction = context.InvokedFunctionStack.at(context.InvokedFunctionStack.size() - 2);
	std::map<StringHandle, DLLInvocationInfo>::const_iterator iter = DLLInvocationMap.find(callingfunction);
	if(iter == DLLInvocationMap.end())
	{
		context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
		return;
	}

	// Now open the DLL and load the corresponding function we wish to invoke
	Marshaling::DLLPool::DLLPoolHandle hdll = Marshaling::TheDLLPool.OpenDLL(iter->second.DLLName);
	if(!hdll)
	{
		context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
		return;
	}

	void* address = Marshaling::DLLPool::GetFunction<void*>(hdll, narrow(iter->second.FunctionName).c_str());
	if(!address)
	{
		context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
		return;
	}


	// Record for tracking what we will push onto the stack for parameters to the external function
	// This is done to minimize the amount of code that must be implemented manually in assembly
	struct pushrec
	{
		UINT_PTR Contents;
		int Is16Bit;

		pushrec(UINT_PTR contents, int is16bit)
			: Contents(contents), Is16Bit(is16bit)
		{ }
	};

	// Track all the stuff we will push onto the native stack
	std::vector<pushrec> stufftopush;

	// Track information for marshaled data structures
	// We don't use a vector here to avoid reallocation of the
	// memory used for tracking the members of the container,
	// which would invalidate the cached buffer pointers
	std::list<MarshaledStructureRecord> marshaledstructures;

	// Booleans are promoted from 1 byte to a full 32-bit dword
	// since that is what most Win32 APIs expect. So we must
	// specifically ensure that we have enough bits available
	// when passing booleans by reference to externals
	std::list<MarshaledBooleanRecord> marshaledbooleans;

	// Cache the index of the function return variable for placing the external
	// function's return value into the appropriate Epoch variable when done
	size_t retindex = std::numeric_limits<size_t>::max();

	// Look for parameters that should be passed on to the external function
	// Remember that we use an Epoch-implemented wrapper function with a special
	// tag to implement this, so the Epoch wrapper's parameters become the actual
	// parameters given to the external function. Note that as a downside of the
	// lack of type information for externals, getting the function signature
	// incorrect can have dire consequences.
	const ScopeDescription& description = context.Variables->GetOriginalDescription();
	for(int i = static_cast<int>(description.GetVariableCount()) - 1; i >= 0; --i)
	{
		VariableOrigin origin = description.GetVariableOrigin(i);
		if(origin == VARIABLE_ORIGIN_PARAMETER)
		{
			EpochTypeID vartype = description.GetVariableTypeByIndex(i);
			StringHandle varname = description.GetVariableNameHandle(i);
			if(vartype > EpochType_CustomBase)
			{
				StructureHandle structurehandle = context.Variables->Read<StructureHandle>(varname);
				ActiveStructure& structure = context.OwnerVM.GetStructure(structurehandle);

				const StructureDefinition& definition = context.OwnerVM.GetStructureDefinition(vartype);
				std::vector<Byte> structbuffer(definition.GetMarshaledSize(), 0);

				if(!MarshalStructureDataIntoBuffer(context, structure, definition, &structbuffer[0]))
				{
					context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
					return;
				}

				marshaledstructures.push_back(MarshaledStructureRecord(description.IsReference(i), structbuffer, structure));
				stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(&marshaledstructures.back().Buffer[0]), false));
			}
			else
			{
				if(description.IsReference(i))
				{
					if(vartype == EpochType_Boolean)
					{
						marshaledbooleans.push_back(MarshaledBooleanRecord(context.Variables->Read<bool>(varname) ? 1 : 0, i));
						stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(&marshaledbooleans.back()), false));
					}
					else if(vartype == EpochType_String || vartype == EpochType_Function || vartype >= EpochType_CustomBase)
					{
						// It is HIGHLY unsafe to pass strings to externals!
						// Therefore if you must pass a mutable string, use a buffer type instead.

						// Functions cannot be passed by reference.

						// Structures should not make it in here since the above clause should catch them
						context.State.Result.ResultType = VM::ExecutionResult::EXEC_RESULT_HALT;
						return;
					}
					else if(vartype == EpochType_Buffer)
					{
						// Allow the buffer to be manipulated directly
						// The programmer is responsibe for ensuring the external does not overflow the buffer!
						BufferHandle handle = context.Variables->Read<BufferHandle>(varname);
						const void* buffer = context.OwnerVM.GetBuffer(handle);
						stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(buffer), false));
					}
					else
						stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(context.Variables->GetVariableStorageLocation(varname)), false));
				}
				else
				{
					switch(vartype)
					{
					case EpochType_Integer:
						stufftopush.push_back(pushrec(context.Variables->Read<Integer32>(varname), false));
						break;

					case EpochType_Integer16:
						stufftopush.push_back(pushrec(context.Variables->Read<Integer16>(varname), true));
						break;

					case EpochType_Real:
						// This is not a mistake: we read the value as an Integer32, but in reality
						// we're holding a float's data. The bits will be garbage if interpreted as
						// an integer, but the value won't be cast to integer and rounded off. This
						// can be thought of as a convoluted wrapper for reinterpret_cast<>.
						stufftopush.push_back(pushrec(context.Variables->Read<Integer32>(varname), false));
						break;

					case EpochType_Boolean:
						stufftopush.push_back(pushrec(context.Variables->Read<bool>(varname) ? 1 : 0, false));
						break;

					case EpochType_String:
						{
							StringHandle handle = context.Variables->Read<StringHandle>(varname);
							const wchar_t* cstr = context.OwnerVM.GetPooledString(handle).c_str();
							stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(cstr), false));
						}
						break;

					case EpochType_Buffer:
						{
							// Hand off a clone of the buffer so that the callee cannot mutate it
							// The garbage collector will reclaim the buffer once the callee returns
							// Therefore, if the buffer must persist beyond the call to the external,
							// ensure that it is passed by reference!
							BufferHandle handle = context.Variables->Read<BufferHandle>(varname);
							const void* buffer = context.OwnerVM.GetBuffer(context.OwnerVM.CloneBuffer(handle));
							stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(buffer), false));
						}
						break;

					case EpochType_Function:
						{
							StringHandle functionname = context.Variables->Read<StringHandle>(varname);
							stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(MarshalControl.RequestMarshaledCallback(context, functionname)), false));
						}
						break;

					default:
						context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
						return;
					}
				}
			}
		}
		else if(origin == VARIABLE_ORIGIN_RETURN)
			retindex = i;
	}

	// Determine the expected return type of the function
	int rettype = EpochType_Void;
	StringHandle retname = 0;
	if(retindex < description.GetVariableCount())
	{
		rettype = description.GetVariableTypeByIndex(retindex);
		retname = description.GetVariableNameHandle(retindex);
	}

	// Push the actual parameters for the function onto the native stack
	if(!stufftopush.empty())
	{
		size_t pushstuffsize = stufftopush.size();
		void* theptr = &stufftopush[0];

		_asm
		{
			mov eax, [theptr]
			mov ecx, pushstuffsize

BatchPushLoop:
			mov edx, eax
			add edx, 4
			cmp [edx], 0
			jnz Push16
			push [eax]
			jmp FinishLoop

Push16:
			mov edx, [eax]
			push dx

FinishLoop:
			add eax, 8
			dec ecx
			jnz BatchPushLoop
		}
	}


	// Call the function and store off the appropriate return value
	_asm
	{
		mov eax, EpochType_Integer
		cmp rettype, eax
		jne TypeIsInteger16
		call address
		mov integerret, eax
		jmp IntegerReturn

TypeIsInteger16:
		mov eax, EpochType_Integer16
		cmp rettype, eax
		jne TypeIsBoolean
		call address
		mov integer16ret, ax
		jmp Integer16Return

TypeIsBoolean:
		mov eax, EpochType_Boolean
		cmp rettype, eax
		jne TypeIsString
		call address
		test eax, eax
		jz BoolIsFalse
		mov integerret, 1
		jmp BooleanReturn
BoolIsFalse:
		mov integerret, 0
		jmp BooleanReturn

TypeIsString:
		mov eax, EpochType_String
		cmp rettype, eax
		jne TypeIsNull
		call address
		mov stringret, eax
		jmp StringReturn

TypeIsNull:
		mov eax, EpochType_Void
		cmp rettype, eax
		jne Vomit
		call address
		jmp NullReturn
	}

IntegerReturn:
	context.Variables->Write<Integer32>(retname, integerret);
	goto MarshalBackToEpoch;

Integer16Return:
	context.Variables->Write<Integer16>(retname, integer16ret);
	goto MarshalBackToEpoch;

BooleanReturn:
	context.Variables->Write<bool>(retname, (integerret != 0 ? true : false));
	goto MarshalBackToEpoch;

StringReturn:
	StringHandle rethandle = context.OwnerVM.PoolString(stringret);
	context.Variables->Write<StringHandle>(retname, rethandle);
	goto MarshalBackToEpoch;

NullReturn:
	goto MarshalBackToEpoch;

Vomit:
	context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
	return;

MarshalBackToEpoch:
	MarshalBuffersIntoStructures(context, marshaledstructures);
	for(std::list<MarshaledBooleanRecord>::const_iterator iter = marshaledbooleans.begin(); iter != marshaledbooleans.end(); ++iter)
		context.Variables->Write(description.GetVariableNameHandle(iter->VariableIndex), iter->Holder != 0);
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
EPOCHVM void VM::MarshalBufferIntoStructureData(VM::ExecutionContext& context, ActiveStructure& structure, const StructureDefinition& definition, const Byte* buffer)
{
	using namespace VM;

	for(size_t j = 0; j < definition.GetNumMembers(); ++j)
	{
		EpochTypeID membertype = definition.GetMemberType(j);
		switch(membertype)
		{
		case EpochType_Integer:
			structure.WriteMember(j, *reinterpret_cast<const Integer32*>(buffer));
			buffer += sizeof(Integer32);
			break;

		case EpochType_Integer16:
			structure.WriteMember(j, *reinterpret_cast<const Integer16*>(buffer));
			buffer += sizeof(Integer16);
			break;

		case EpochType_Boolean:
			structure.WriteMember(j, (*reinterpret_cast<const Integer32*>(buffer)) ? true : false);
			buffer += sizeof(Integer32);
			break;

		case EpochType_String:
			{
				const wchar_t* const* ptr = reinterpret_cast<const wchar_t* const*>(buffer);
				if(ptr && *ptr)
				{
					std::wstring str(*ptr);
					structure.WriteMember(j, context.OwnerVM.PoolString(str));
				}
				else
					structure.WriteMember(j, context.OwnerVM.PoolString(L""));
				buffer += sizeof(const wchar_t*);
			}
			break;

		case EpochType_Function:
			// Function pointers are not marshaled back to Epoch form, currently.
			// This would require an additional interop layer that dynamically
			// relinks Epoch callback functions into their modified targets.
			buffer += sizeof(void*);
			break;

		case EpochType_Buffer:
			// Buffers are passed as pointers to raw data and if mutated by the
			// external function will not need to be marshaled back manually
			buffer += sizeof(void*);
			break;

		default:
			if(membertype > EpochType_CustomBase)
			{
				StructureHandle structurehandle = structure.ReadMember<StructureHandle>(j);
				ActiveStructure& nestedstructure = context.OwnerVM.GetStructure(structurehandle);
				const StructureDefinition& nesteddefinition = context.OwnerVM.GetStructureDefinition(definition.GetMemberType(j));

				MarshalBufferIntoStructureData(context, nestedstructure, nesteddefinition, buffer);
				buffer += nesteddefinition.GetMarshaledSize();
			}
			else
			{
				context.State.Result.ResultType = VM::ExecutionResult::EXEC_RESULT_HALT;
				return;
			}
		}
	}
}
