//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for external (DLL-based) functions
//

#include "pch.h"

#include "Library Functionality/Function Tags/Externals.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ActiveScope.h"

#include "Utility/DLLPool.h"

#include "Utility/Strings.h"

#include <limits>


namespace
{
	UInteger32 __stdcall CallbackInvoke(UByte* espsave, VM::ExecutionContext* context, StringHandle callbackfunction);


	struct DLLInvocationInfo
	{
		std::wstring DLLName;
		std::wstring FunctionName;
	};

	std::map<StringHandle, DLLInvocationInfo> DLLInvocationMap;

	//
	// Helper stub for saving stack information when a callback is invoked.
	// This information is used to read parameters off the stack.
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

		struct StubSpaceRecord
		{
			void* StartOfSpace;
			void* NextAvailableSlot;
		};

		std::vector<StubSpaceRecord> StubSpaceList;

		Threads::CriticalSection MarshalingCriticalSection;

	public:
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
		// Helper functions which allocate memory as needed for dynamically generated shims
		//
		void* AllocNewStubSpace()
		{
			StubSpaceRecord rec;
			rec.StartOfSpace = ::VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			rec.NextAvailableSlot = rec.StartOfSpace;
			StubSpaceList.push_back(rec);
			return rec.NextAvailableSlot;
		}

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
		void* RequestMarshaledCallback(VM::ExecutionContext& context, StringHandle callbackfunction)
		{
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

	MarshalingController MarshalControl;


	//
	// Actually invoke the Epoch function, taking care to
	// provide the correct parameters from the stack.
	//
	UInteger32 __stdcall CallbackInvoke(UByte* espsave, VM::ExecutionContext* context, StringHandle callbackfunction)
	{
		// We need to increment past the stored value of ESP
		// in order to reach the actual first parameter. This
		// is because of the local variable defined in the
		// callback stub, which uses a stack slot.
		UByte* esp = espsave + 4;

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
					context->State.Stack.PushValue<bool>(*reinterpret_cast<bool*>(esp));
					esp += sizeof(bool);
					break;

				case VM::EpochType_Integer:
					context->State.Stack.PushValue<Integer32>(*reinterpret_cast<Integer32*>(esp));
					esp += sizeof(Integer32);
					break;

				case VM::EpochType_Real:
					context->State.Stack.PushValue<Real32>(*reinterpret_cast<Real32*>(esp));
					esp += sizeof(Real32);
					break;

				case VM::EpochType_String:
					{
						StringHandle handle = context->OwnerVM.PoolString(*reinterpret_cast<wchar_t**>(esp));
						context->State.Stack.PushValue<StringHandle>(handle);
						esp += sizeof(StringHandle);
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
		// value (in the EAX register, as per __stdcall). In
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
	// Dispatch execution to an external DLL function
	//
	void ExternalDispatch(StringHandle functionname, VM::ExecutionContext& context)
	{
		using namespace VM;

		StringHandle callingfunction = context.InvokedFunctionStack.c.at(context.InvokedFunctionStack.c.size() - 2);
		std::map<StringHandle, DLLInvocationInfo>::const_iterator iter = DLLInvocationMap.find(callingfunction);
		if(iter == DLLInvocationMap.end())
		{
			context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
			return;
		}

		HINSTANCE hdll = Marshaling::TheDLLPool.OpenDLL(iter->second.DLLName);
		if(!hdll)
		{
			context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
			return;
		}

		void* address = ::GetProcAddress(hdll, narrow(iter->second.FunctionName).c_str());
		if(!address)
		{
			context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
			return;
		}

		struct pushrec
		{
			UINT_PTR Contents;
			int Is16Bit;

			pushrec(UINT_PTR contents, int is16bit)
				: Contents(contents), Is16Bit(is16bit)
			{ }
		};
		std::vector<pushrec> stufftopush;

		Integer32 integerret;

		size_t retindex = std::numeric_limits<size_t>::max();

		const ScopeDescription& description = context.Variables->GetOriginalDescription();
		for(int i = static_cast<int>(description.GetVariableCount()) - 1; i >= 0; --i)
		{
			VariableOrigin origin = description.GetVariableOrigin(i);
			if(origin == VARIABLE_ORIGIN_PARAMETER)
			{
				switch(description.GetVariableTypeByIndex(i))
				{
				case EpochType_Integer:
					stufftopush.push_back(pushrec(context.Variables->Read<Integer32>(description.GetVariableNameHandle(i)), false));
					break;

				case EpochType_Real:
					// This is not a mistake: we read the value as an Integer32, but in reality
					// we're holding a float's data. The bits will be garbage if interpreted as
					// an integer, but the value won't be cast to integer and rounded off.
					stufftopush.push_back(pushrec(context.Variables->Read<Integer32>(description.GetVariableNameHandle(i)), false));
					break;

				case EpochType_Boolean:
					stufftopush.push_back(pushrec(context.Variables->Read<bool>(description.GetVariableNameHandle(i)) ? 1 : 0, false));
					break;

				case EpochType_String:
					{
						StringHandle handle = context.Variables->Read<StringHandle>(description.GetVariableNameHandle(i));
						const wchar_t* cstr = context.OwnerVM.GetPooledString(handle).c_str();
						stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(cstr), false));
					}
					break;

				case EpochType_Buffer:
					{
						BufferHandle handle = context.Variables->Read<BufferHandle>(description.GetVariableNameHandle(i));
						const void* buffer = context.OwnerVM.GetBuffer(handle);
						stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(buffer), false));
					}
					break;

				case EpochType_Function:
					{
						StringHandle functionname = context.Variables->Read<StringHandle>(description.GetVariableNameHandle(i));
						stufftopush.push_back(pushrec(reinterpret_cast<UINT_PTR>(MarshalControl.RequestMarshaledCallback(context, functionname)), false));
					}
					break;

				default:
					context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
					return;
				}
			}
			else if(origin == VARIABLE_ORIGIN_RETURN)
				retindex = i;
		}

		int rettype = EpochType_Void;
		if(retindex < description.GetVariableCount())
			rettype = description.GetVariableTypeByIndex(retindex);

		if(!stufftopush.empty())
		{
			size_t pushstuffsize = stufftopush.size();
			void* theptr = &stufftopush[0];

			_asm
			{
				mov ebx, address

				mov eax, [theptr]
				mov edx, eax
				add edx, 4
				mov ecx, pushstuffsize

BatchPushLoop:
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
		else
			__asm mov ebx, address


		_asm
		{
			cmp rettype, EpochType_Integer
			jne TypeIsBoolean
			call ebx
			mov integerret, eax
			jmp IntegerReturn

TypeIsBoolean:
			cmp rettype, EpochType_Boolean
			jne TypeIsNull
			call ebx
			test eax, eax
			jz BoolIsFalse
			mov integerret, 1
			jmp BooleanReturn
BoolIsFalse:
			mov integerret, 0
			jmp BooleanReturn

TypeIsNull:
			cmp rettype, EpochType_Void
			jne Vomit
			call ebx
			jmp NullReturn
		}

IntegerReturn:
		context.Variables->Write<Integer32>(context.OwnerVM.PoolString(L"ret"), integerret);
		return;

BooleanReturn:
		context.Variables->Write<bool>(context.OwnerVM.PoolString(L"ret"), (integerret != 0 ? true : false));
		return;

NullReturn:
		return;

Vomit:
		context.State.Result.ResultType = ExecutionResult::EXEC_RESULT_HALT;
	}

	//
	// Register a function tagged as an external
	//
	TagHelperReturn ExternalHelper(StringHandle functionname, const CompileTimeParameterVector& compiletimeparams, bool isprepass)
	{
		TagHelperReturn ret;

		if(isprepass)
		{
			DLLInvocationInfo info;
			info.DLLName = compiletimeparams[0].StringPayload;
			info.FunctionName = compiletimeparams[1].StringPayload;
			DLLInvocationMap[functionname] = info;
		}
		else
		{
			ret.InvokeRuntimeFunction = L"@@external";
		}

		return ret;
	}

}


//
// Bind the library to an execution dispatch table
//
void FunctionTags::RegisterExternalTag(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"@@external"), FunctionSignature()));
}

//
// Bind the library to a function metadata table
//
void FunctionTags::RegisterExternalTag(FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"@@external"), ExternalDispatch));
}


//
// Bind the library's tag helpers to the compiler
//
void FunctionTags::RegisterExternalTagHelper(FunctionTagHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"external", ExternalHelper));
}

