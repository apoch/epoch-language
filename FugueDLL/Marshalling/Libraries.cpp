//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Routines for binding to external libraries
//

#include "pch.h"

#include "Marshalling/Libraries.h"
#include "Marshalling/DLLPool.h"
#include "Marshalling/ExternalDLL.h"
#include "Marshalling/LibraryImporting.h"

#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/VMExceptions.h"

#include "Utility/Strings.h"



//
// Structure for tracking what code we need to bind to
//
struct BindRec
{
	VM::Program* TheProgram;
	const wchar_t* DLLName;
};

//
// Handy function pointer type shortcuts
//
typedef void (__stdcall *VMLinkPtr)(RegistrationTable table, void* bindrecord);


//
// This function is called by the external library DLL
// to register its functions with the VM.
//
void __stdcall RegistrationFunc(const wchar_t* name, const char* internalname, const ParamData* params, size_t numparams, VM::EpochVariableTypeID returntype, VM::EpochVariableTypeID returntypehint, void* bindrecord)
{
	BindRec* realbindrecord = reinterpret_cast<BindRec*>(bindrecord);

	std::auto_ptr<VM::ScopeDescription> paramscope(new VM::ScopeDescription);
	const ParamData* p = params;
	for(UInteger32 i = 0; i < numparams; ++i)
		paramscope->AddVariable(p[i].Name, p[i].Type);

	VM::ScopeDescription& scope = realbindrecord->TheProgram->GetGlobalScope();
	std::auto_ptr<VM::FunctionBase> func(new Marshalling::CallDLL(realbindrecord->DLLName, widen(internalname), paramscope.release(), returntype, returntypehint));
	scope.AddFunction(name, func);
}

//
// This function is called by the external library DLL
// to register constants with the VM.
//
void __stdcall RegistrationConstant(const wchar_t* name, VM::EpochVariableTypeID type, void* value, void* bindrecord)
{
	BindRec* realbindrecord = reinterpret_cast<BindRec*>(bindrecord);

	realbindrecord->TheProgram->GetGlobalScope().AddVariable(name, type);
	realbindrecord->TheProgram->GetGlobalScope().SetConstant(name);

	std::auto_ptr<VM::Operation> pushop(NULL);
	switch(type)
	{
	case VM::EpochVariableType_Boolean:
		pushop.reset(new VM::Operations::PushBooleanLiteral(*reinterpret_cast<bool*>(value)));
		break;

	case VM::EpochVariableType_Integer:
		pushop.reset(new VM::Operations::PushIntegerLiteral(*reinterpret_cast<Integer32*>(value)));
		break;

	case VM::EpochVariableType_Integer16:
		pushop.reset(new VM::Operations::PushInteger16Literal(*reinterpret_cast<Integer16*>(value)));
		break;

	case VM::EpochVariableType_Real:
		pushop.reset(new VM::Operations::PushRealLiteral(*reinterpret_cast<Real*>(value)));
		break;

	case VM::EpochVariableType_String:
		pushop.reset(new VM::Operations::PushStringLiteral(realbindrecord->TheProgram->PoolStaticString(reinterpret_cast<wchar_t*>(value))));
		break;

	default:
		throw VM::NotImplementedException("Cannot register constant of this type");
	}

	realbindrecord->TheProgram->CreateGlobalInitBlock().AddOperation(pushop);
	realbindrecord->TheProgram->CreateGlobalInitBlock().AddOperation(VM::OperationPtr(new VM::Operations::InitializeValue(realbindrecord->TheProgram->PoolStaticString(name))));
}

//
// This function is called by the external library DLL
// to register structure types with the VM.
//
IDType __stdcall RegistrationStructure(const wchar_t* name, const ParamData* params, size_t numparams, void* bindrecord)
{
	BindRec* realbindrecord = reinterpret_cast<BindRec*>(bindrecord);

	VM::StructureType typeinfo;
	for(size_t i = 0; i < numparams; ++i)
	{
		if(params[i].Type == VM::EpochVariableType_Structure)
		{
			const VM::StructureType& structinfo = realbindrecord->TheProgram->GetGlobalScope().GetStructureType(params[i].Hint);
			typeinfo.AddMember(params[i].Name, structinfo, params[i].Hint);
		}
		else if(params[i].Type == VM::EpochVariableType_Function)
			typeinfo.AddFunctionMember(params[i].Name, params[i].StringHint);
		else
			typeinfo.AddMember(params[i].Name, params[i].Type);
	}

	typeinfo.ComputeOffsets(realbindrecord->TheProgram->GetGlobalScope());

	realbindrecord->TheProgram->GetGlobalScope().AddStructureType(name, typeinfo);
	return realbindrecord->TheProgram->GetGlobalScope().GetStructureTypeID(name);
}

//
// This function is called by the external library DLL
// to register an external DLL function with the VM.
//
// Note that this external function can be in ANY DLL,
// not just the library DLL itself.
//
void __stdcall RegistrationExternal(const wchar_t* name, const char* internalname, const wchar_t* dllname, const ParamData* params, size_t numparams, VM::EpochVariableTypeID returntype, void* bindrecord)
{
	BindRec* realbindrecord = reinterpret_cast<BindRec*>(bindrecord);

	VM::ScopeDescription& scope = realbindrecord->TheProgram->GetGlobalScope();

	std::auto_ptr<VM::ScopeDescription> paramscope(new VM::ScopeDescription);
	const ParamData* p = params;

	// We have to register the parameters backwards to properly follow __stdcall
	size_t i = numparams;
	do
	{
		--i;

		if(p[i].IsReference)
		{
			paramscope->AddReference(p[i].Type, p[i].Name);

			if(p[i].Type == VM::EpochVariableType_Structure)
				paramscope->SetVariableStructureTypeID(p[i].Name, p[i].Hint);
		}
		else
		{
			if(p[i].Type == VM::EpochVariableType_Structure)
				paramscope->AddStructureVariable(p[i].Hint, p[i].Name);
			else
				paramscope->AddVariable(p[i].Name, p[i].Type);
		}
	} while(i > 0);

	std::auto_ptr<VM::FunctionBase> func(new Marshalling::CallDLL(dllname, widen(internalname), paramscope.release(), returntype, VM::EpochVariableType_Error));
	scope.AddFunction(name, func);
}

//
// This function is called by the external library DLL
// to register a function signature type with the VM.
//
void __stdcall RegistrationSignature(const wchar_t* name, const ParamData* params, size_t numparams, VM::EpochVariableTypeID returntype, void* bindrecord)
{
	BindRec* realbindrecord = reinterpret_cast<BindRec*>(bindrecord);

	VM::ScopeDescription& scope = realbindrecord->TheProgram->GetGlobalScope();

	VM::FunctionSignature signature;
	for(UInteger32 i = 0; i < numparams; ++i)
		signature.AddParam(params->Type, params->Hint, NULL);

	signature.AddReturn(returntype, 0);

	scope.AddFunctionSignature(name, signature, true);
}


void* __stdcall RequestMarshalBuffer(size_t numbytes)
{
	return new char[numbytes];
}


//
// Call the specified DLL and ask it to register with the VM
//
void Marshalling::BindToLibrary(const std::wstring& filename, VM::Program& program)
{
	std::wstring fullfilename = filename + L".dll";

	if(Marshalling::TheDLLPool.HasOpenedDLL(fullfilename))
		return;

	HINSTANCE hdll = Marshalling::TheDLLPool.OpenDLL(fullfilename);
	VMLinkPtr vmlink = reinterpret_cast<VMLinkPtr>(::GetProcAddress(hdll, "LinkToEpochVM"));
	if(!vmlink)
		throw VM::ExecutionException("Cannot link to library - does not support Epoch bindings. Use \"external\" declarations instead.");

	RegistrationTable regtable;
	regtable.RegisterFunction = RegistrationFunc;
	regtable.RegisterConstant = RegistrationConstant;
	regtable.RegisterStructure = RegistrationStructure;
	regtable.RegisterExternal = RegistrationExternal;
	regtable.RegisterSignature = RegistrationSignature;

	regtable.RequestMarshalBuffer = RequestMarshalBuffer;

	BindRec bindrec;
	bindrec.TheProgram = &program;
	bindrec.DLLName = fullfilename.c_str();

	vmlink(regtable, &bindrec);
}


//
// Language extensions might offer some bindings, but are not required to;
// therefore we take a slightly different approach to loading them, versus
// a standard library DLL.
//
void Marshalling::BindToLanguageExtension(const std::wstring& filename, VM::Program& program)
{
	std::wstring fullfilename = filename + L".dll";

	if(Marshalling::TheDLLPool.HasOpenedDLL(fullfilename))
		return;

	HINSTANCE hdll = Marshalling::TheDLLPool.OpenDLL(fullfilename);
	VMLinkPtr vmlink = reinterpret_cast<VMLinkPtr>(::GetProcAddress(hdll, "LinkToEpochVM"));
	if(!vmlink)
		return;

	RegistrationTable regtable;
	regtable.RegisterFunction = RegistrationFunc;
	regtable.RegisterConstant = RegistrationConstant;
	regtable.RegisterStructure = RegistrationStructure;
	regtable.RegisterExternal = RegistrationExternal;
	regtable.RegisterSignature = RegistrationSignature;

	regtable.RequestMarshalBuffer = RequestMarshalBuffer;

	BindRec bindrec;
	bindrec.TheProgram = &program;
	bindrec.DLLName = fullfilename.c_str();

	vmlink(regtable, &bindrec);
}

