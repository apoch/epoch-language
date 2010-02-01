//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Marshalling system for external C-style callbacks into Epoch code
//
// WARNING - all marshalling code is platform-specific!
//
// In order to allow external code to call into an Epoch function, we
// set up a thunk table to redirect the call into the VM. For each
// function that gets called from the outside, we allocate a chunk
// of memory. In that chunk, we generate some machine code that
// takes care of invoking the desired Epoch function. Then, we use
// that chunk's address as the "address" of the callback function.
// This effectively allows external code to call Epoch transparently.
//

#include "pch.h"

#include "Marshalling/Callback.h"

#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/RValue.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"

#include "Virtual Machine/Types Management/Typecasts.h"

#include "Virtual Machine/VMExceptions.h"

#include "Utility/Threading/Threads.h"
#include "Utility/Threading/Synchronization.h"



namespace
{

	//
	// Track the mapping between generated callback stubs and
	// the Epoch functions they are intended to invoke
	//
	typedef std::map<VM::Function*, void*> MarshallingMapT;
	MarshallingMapT MarshalledCallbackMap;

	struct StubSpaceRecord
	{
		void* StartOfSpace;
		void* NextAvailableSlot;
	};

	std::vector<StubSpaceRecord> StubSpaceList;

	Threads::CriticalSection MarshallingCriticalSection;


	//
	// Actually invoke the Epoch function, taking care to
	// provide the correct parameters from the stack.
	//
	UInteger32 __stdcall CallbackInvoke(UByte* espsave, VM::Function* callbackfunction)
	{
		// We need to increment past the stored value of ESP
		// in order to reach the actual first parameter. This
		// is because of the local variable defined in the
		// callback stub, which uses a stack slot.
		std::auto_ptr<VM::ActivatedScope> tempscope(new VM::ActivatedScope(VM::GetRunningProgram()->GetGlobalScope()));
		VM::RValuePtr result(callbackfunction->InvokeWithExternalParams(VM::GetRunningProgram()->GetStack(), espsave + 4, *tempscope));

		// The Epoch function's return value is used to determine
		// how we leave this shim. We need to clean up local
		// variables from the stack as well as pass on the return
		// value (in the EAX register, as per __stdcall). In
		// order to make sure all the housekeeping is done correctly,
		// we let the compiler generate the return code for us.
		switch(result->GetType())
		{
		case VM::EpochVariableType_Null:
			return 0;

		case VM::EpochVariableType_Integer:
			return static_cast<UInteger32>(result->CastTo<VM::IntegerRValue>().GetValue());

		case VM::EpochVariableType_Integer16:
			return static_cast<UInteger32>(result->CastTo<VM::Integer16RValue>().GetValue());

		case VM::EpochVariableType_Boolean:
			return static_cast<UInteger32>(result->CastTo<VM::BooleanRValue>().GetValue() ? 1 : 0);

		default:
			throw VM::NotImplementedException("Support for return values of this type from a callback is not yet implemented!");
		}

		return 0;
	}

	//
	// Helper stub for saving stack information when a callback is invoked.
	// This information is used to read parameters off the stack.
	//
	void __declspec(naked) CallbackEntryPoint()
	{
		void* espsave;
		__asm mov espsave, esp;

		__asm push edx;
		__asm push espsave;
		__asm call CallbackInvoke;
		__asm ret;
	}


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

}


//
// Request the marshalling system to provide a callback stub for the
// given Epoch function. The address of the stub is returned and can
// be handed off to external code as the callback function.
//
void* Marshalling::RequestMarshalledCallback(VM::Function* callbackfunction)
{
	Threads::CriticalSection::Auto mutex(MarshallingCriticalSection);

	// Check if we have already generated a stub for this function
	MarshallingMapT::const_iterator iter = MarshalledCallbackMap.find(callbackfunction);
	if(iter != MarshalledCallbackMap.end())
		return iter->second;

	// Generate a new callback stub
	void* stubspace = GetStubSpace();

	UByte* rawbytes = reinterpret_cast<UByte*>(stubspace);
	UINT_PTR stubaddress = reinterpret_cast<UINT_PTR>(CallbackEntryPoint);
	UINT_PTR callbackaddress = reinterpret_cast<UINT_PTR>(callbackfunction);

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

	// jmp ecx
	rawbytes[10] = 0xFF;
	rawbytes[11] = 0xE1;

	MarshalledCallbackMap[callbackfunction] = &rawbytes[0];

	return (&rawbytes[0]);
}

//
// Clean up and reset the marshalling layer (discards all generated stubs)
//
void Marshalling::Clean()
{
	Threads::CriticalSection::Auto mutex(MarshallingCriticalSection);

	MarshalledCallbackMap.clear();

	for(std::vector<StubSpaceRecord>::iterator iter = StubSpaceList.begin(); iter != StubSpaceList.end(); ++iter)
		::VirtualFree(iter->StartOfSpace, 0, MEM_RELEASE);

	StubSpaceList.clear();
}


