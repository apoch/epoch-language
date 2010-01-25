//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Routines for integrating with external DLLs
//
// WARNING - all marshalling code is platform-specific!
//

#include "pch.h"

#include "Marshalling/ExternalDLL.h"
#include "Marshalling/Callback.h"
#include "Marshalling/StructureConversion.h"
#include "Marshalling/DLLPool.h"

#include "Utility/Strings.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"
#include "Virtual Machine/SelfAware.inl"


using namespace Marshalling;
using namespace VM;


//
// Construct a DLL call operation
//
CallDLL::CallDLL(const std::wstring& dllname, const std::wstring& functionname, ScopeDescription* paramlist, EpochVariableTypeID returntype)
	: DLLName(dllname), FunctionName(functionname), Params(paramlist), ReturnType(returntype)
{
}

//
// Destruct and clean up a DLL call operation
//
CallDLL::~CallDLL()
{
	delete Params;
}


//
// Invoke a function in a separate DLL
//
// Note that we must do the invocation by hand in assembly, because
// pushing parameters onto the machine stack disrupts the ESP pointer,
// which in turn disrupts accessing local variables in the current
// stack frame. To avoid this, we save off the information we need
// into registers, and work from there to invoke the function.
//
RValuePtr CallDLL::Invoke(StackSpace& stack, ActivatedScope& scope)
{
	class AutoDeleter
	{
	public:
		~AutoDeleter()
		{
			for(std::vector<Byte*>::iterator iter = BufferSet.begin(); iter != BufferSet.end(); ++iter)
				delete [] *iter;
		}

	public:
		std::vector<Byte*> BufferSet;
		std::vector<VM::StructureVariable*> Variables;
	} Buffers;

	std::auto_ptr<ActivatedScope> paramclone(new ActivatedScope(*Params));
	paramclone->BindToStack(stack);

	Integer32 integerret;
	Integer16 integer16ret;

	HINSTANCE hdll = TheDLLPool.OpenDLL(DLLName);
	if(!hdll)
		throw ExecutionException("Invalid DLL call - could not load library");

	void* address = ::GetProcAddress(hdll, narrow(FunctionName).c_str());
	if(!address)
		throw ExecutionException("Invalid DLL call - could not locate function");

	struct pushrec
	{
		UINT_PTR Contents;
		int Is16Bit;

		pushrec(UINT_PTR contents, int is16bit)
			: Contents(contents), Is16Bit(is16bit)
		{ }
	};
	std::vector<pushrec> StuffToPush;

	const std::vector<std::wstring>& paramorder = Params->GetMemberOrder();
	for(std::vector<std::wstring>::const_iterator iter = paramorder.begin(); iter != paramorder.end(); ++iter)
	{
		switch(paramclone->GetVariableType(*iter))
		{
		case EpochVariableType_Integer:
			{
				Integer32 intval = paramclone->GetVariableRef<VM::IntegerVariable>(*iter).GetValue();
				StuffToPush.push_back(pushrec(intval, false));
			}
			break;

		case EpochVariableType_Integer16:
			{
				Integer16 intval = paramclone->GetVariableRef<VM::Integer16Variable>(*iter).GetValue();
				StuffToPush.push_back(pushrec(intval, true));
			}

		case EpochVariableType_String:
			{
				const wchar_t* strval = paramclone->GetVariableRef<VM::StringVariable>(*iter).GetValue().c_str();
				StuffToPush.push_back(pushrec(reinterpret_cast<UINT_PTR>(strval), false));
			}
			break;

		case EpochVariableType_Structure:
			{
				VM::StructureVariable& structvar = paramclone->GetVariableRef<VM::StructureVariable>(*iter);
				void* buffer = EpochToCStruct(structvar, *paramclone);
				Buffers.BufferSet.push_back(reinterpret_cast<Byte*>(buffer));
				Buffers.Variables.push_back(&structvar);
				StuffToPush.push_back(pushrec(reinterpret_cast<UINT_PTR>(buffer), false));
			}
			break;

		case EpochVariableType_Function:
			{
				Function* func = dynamic_cast<Function*>(paramclone->GetFunction(*iter));
				if(!func)
					throw ExecutionException("Only user defined functions can be passed to external DLL functions");

				void* marshallptr = RequestMarshalledCallback(func);
				StuffToPush.push_back(pushrec(reinterpret_cast<UINT_PTR>(marshallptr), false));
			}
			break;

		case EpochVariableType_Buffer:
			{
				Byte* bufferptr = paramclone->GetVariableRef<VM::BufferVariable>(*iter).GetValue();
				StuffToPush.push_back(pushrec(reinterpret_cast<UINT_PTR>(bufferptr), false));
			}
			break;

		default:
			throw ExecutionException("Cannot pass this argument type to a DLL call");
		}
	}

	EpochVariableTypeID rettype = ReturnType;

	if(!StuffToPush.empty())
	{
		size_t pushstuffsize = StuffToPush.size();
		void* theptr = &StuffToPush[0];

		_asm
		{
			mov ebx, address		// We do this here just in case one of the above function calls uses ebx

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
		cmp rettype, EpochVariableType_Integer
		jne TypeIsInteger16
		call ebx
		mov integerret, eax
		jmp IntegerReturn

TypeIsInteger16:
		cmp rettype, EpochVariableType_Integer16
		jne TypeIsBoolean
		call ebx
		mov integer16ret, ax
		jmp Integer16Return

TypeIsBoolean:
		cmp rettype, EpochVariableType_Boolean
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
		cmp rettype, EpochVariableType_Null
		jne Vomit
		call ebx
		jmp NullReturn
	}

IntegerReturn:
	CStructToEpoch(Buffers.BufferSet, Buffers.Variables);
	paramclone->Exit(stack);
	return RValuePtr(new IntegerRValue(integerret));

Integer16Return:
	CStructToEpoch(Buffers.BufferSet, Buffers.Variables);
	paramclone->Exit(stack);
	return RValuePtr(new Integer16RValue(integer16ret));

BooleanReturn:
	CStructToEpoch(Buffers.BufferSet, Buffers.Variables);
	paramclone->Exit(stack);
	return RValuePtr(new BooleanRValue(integerret ? true : false));

NullReturn:
	CStructToEpoch(Buffers.BufferSet, Buffers.Variables);
	paramclone->Exit(stack);
	return RValuePtr(new NullRValue);

Vomit:
	throw VM::NotImplementedException("Not sure what to do with return value from DLL call; no function call was made");
}


//
// Helper function for traversing a DLL invocation operation
//
// We template this function in order to eliminate code duplication,
// since all traversers should handle the operation the same way.
//
template <typename TraverserT>
void CallDLL::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);

	if(Params)
		Params->Traverse(traverser);
}

//
// Traverse the call for validation purposes
//
void CallDLL::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

//
// Traverse the call for serialization purposes
//
void CallDLL::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

