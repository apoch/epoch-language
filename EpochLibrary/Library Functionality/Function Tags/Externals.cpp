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

	struct DLLInvocationInfo
	{
		std::wstring DLLName;
		std::wstring FunctionName;
	};

	std::map<StringHandle, DLLInvocationInfo> DLLInvocationMap;


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

