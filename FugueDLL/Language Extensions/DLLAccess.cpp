//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper logic for accessing language extension libraries
//

#include "pch.h"

#include "Language Extensions/DLLAccess.h"
#include "Language Extensions/ExtensionCatalog.h"

#include "Traverser/TraversalInterface.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Variables/ArrayVariable.h"

#include "Marshalling/DLLPool.h"

#include "Utility/Strings.h"


using namespace Extensions;



//
// Construct the access wrapper and initialize the DLL bindings
//
ExtensionDLLAccess::ExtensionDLLAccess(const std::wstring& dllname, VM::Program& program, bool startsession)
	: SessionHandle(0),
	  DLLName(dllname),
	  ExtensionValid(false),
	  DLLHandle(NULL)
{
	// Load the DLL
	DLLHandle = Marshalling::TheDLLPool.OpenDLL(dllname);
	if(!DLLHandle)
		throw Exception("A language extension DLL was requested, but the DLL was either not found or reported some error during initialization");

	// Obtain interface into DLL
	DoInitialize = reinterpret_cast<InitializePtr>(::GetProcAddress(DLLHandle, "Initialize"));
	DoRegistration = reinterpret_cast<RegistrationPtr>(::GetProcAddress(DLLHandle, "Register"));
	DoLoadSource = reinterpret_cast<LoadSourceBlockPtr>(::GetProcAddress(DLLHandle, "LoadSourceBlock"));
	DoExecuteSource = reinterpret_cast<ExecuteSourceBlockPtr>(::GetProcAddress(DLLHandle, "ExecuteSourceBlock"));
	DoExecuteControl = reinterpret_cast<ExecuteControlPtr>(::GetProcAddress(DLLHandle, "ExecuteControl"));
	DoPrepare = reinterpret_cast<PreparePtr>(::GetProcAddress(DLLHandle, "CommitCompilation"));
	DoStartSession = reinterpret_cast<StartCompileSessionPtr>(::GetProcAddress(DLLHandle, "StartNewProgramCompilation"));
	DoFillSerializationBuffer = reinterpret_cast<FillSerializationBufferPtr>(::GetProcAddress(DLLHandle, "FillSerializationBuffer"));
	DoFreeSerializationBuffer = reinterpret_cast<FreeSerializationBufferPtr>(::GetProcAddress(DLLHandle, "FreeSerializationBuffer"));
	DoLoadDataBuffer = reinterpret_cast<LoadDataBufferPtr>(::GetProcAddress(DLLHandle, "LoadDataBuffer"));
	DoPrepareBlock = reinterpret_cast<PrepareBlockPtr>(::GetProcAddress(DLLHandle, "PrepareBlock"));

	// Validate interface to be sure
	if(!DoInitialize || !DoRegistration || !DoLoadSource || !DoExecuteSource || !DoExecuteControl || !DoPrepare || !DoStartSession || !DoFillSerializationBuffer || !DoFreeSerializationBuffer || !DoLoadDataBuffer || !DoPrepareBlock)
		throw Exception("One or more Epoch service functions could not be loaded from the requested language extension DLL");

	ExtensionValid = DoInitialize();

	if(startsession)
		SessionHandle = DoStartSession(reinterpret_cast<HandleType>(&program));
	else
		SessionHandle = 0;
}


//
// Create a block of code in the language extension, given a block of raw Epoch code
//
CodeBlockHandle ExtensionDLLAccess::LoadSourceBlock(const std::wstring& keyword, OriginalCodeHandle handle)
{
	return DoLoadSource(SessionHandle, handle, keyword.c_str());
}

//
// Invoke the code generated for the specified language extension code block
//
void ExtensionDLLAccess::ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle)
{
	return DoExecuteSource(handle, activatedscopehandle);
}

void ExtensionDLLAccess::ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle, const std::vector<Traverser::Payload>& payloads)
{
	if(payloads.empty())
		return DoExecuteControl(handle, activatedscopehandle, 0, NULL);
	else
		return DoExecuteControl(handle, activatedscopehandle, payloads.size(), &payloads[0]);
}


namespace
{

	//
	// Callback: register an extension keyword for this language extension
	//
	// The language extension invokes this callback for each keyword it wishes
	// to add to the language. When one of these keywords is encountered, the
	// subsequent code block is converted by the extension library.
	//
	void __stdcall RegistrationCallback(ExtensionLibraryHandle token, const wchar_t* keyword)
	{
		RegisterExtensionKeyword(keyword, token);
	}

	void __stdcall ControlRegistrationCallback(ExtensionLibraryHandle token, const wchar_t* keyword, size_t numparams, ExtensionControlParamInfo* params)
	{
		RegisterExtensionControl(keyword, token, numparams, params);
	}


	class BindTraversal
	{
	public:
		BindTraversal(Traverser::Interface* traversal, HandleType sessionhandle)
			: Traversal(traversal),
			  SessionHandle(sessionhandle)
		{ }

	public:
		void EnterBlock(const VM::Block& block)
		{
			ActiveScopes.push(block.GetBoundScope());
			Traversal->NodeEntryCallback(SessionHandle);
		}

		void ExitBlock(const VM::Block& block)
		{
			Traversal->NodeExitCallback(SessionHandle);
			ActiveScopes.pop();
		}

		void TraverseNode(const std::wstring& token, const Traverser::Payload& payload)
		{
			Traversal->NodeTraversalCallback(SessionHandle, token.c_str(), &payload);
		}

		void TraverseFunction(const std::wstring& funcname, VM::Function* targetfunction)
		{
			Traversal->FunctionTraversalCallback(SessionHandle, funcname.c_str());
			targetfunction->GetReturns().TraverseExternal(*this);
			targetfunction->GetParams().TraverseExternal(*this);
			targetfunction->GetCodeBlock()->TraverseExternal(*this);
		}

		void RegisterScope(const VM::ScopeDescription* description, bool isghost)
		{
			ActiveScopes.push(description);

			std::vector<Traverser::ScopeContents> contents;
			for(std::vector<std::wstring>::const_iterator iter = description->GetMemberOrder().begin(); iter != description->GetMemberOrder().end(); ++iter)
			{
				Traverser::ScopeContents content;
				content.Identifier = iter->c_str();
				content.Type = description->GetVariableType(*iter);

				if(content.Type == VM::EpochVariableType_Array)
				{
					content.ContainedType = description->GetArrayType(*iter);
					content.ContainedSize = 0;
				}

				contents.push_back(content);
			}

			Traversal->ScopeTraversalCallback(SessionHandle, ActiveScopes.size() == 1, isghost, contents.size(), contents.size() ? &(contents[0]) : NULL);

			std::set<const VM::ScopeDescription*> ghostscopes = description->GetAllGhostScopes();
			for(std::set<const VM::ScopeDescription*>::const_iterator iter = ghostscopes.begin(); iter != ghostscopes.end(); ++iter)
				RegisterScope(*iter, true);

			ActiveScopes.pop();
		}

		const VM::ScopeDescription* GetCurrentScope() const
		{
			if(ActiveScopes.empty())
				return NULL;

			return ActiveScopes.top();
		}

	private:
		Traverser::Interface* Traversal;
		HandleType SessionHandle;
		std::stack<const VM::ScopeDescription*> ActiveScopes;
	};


	//
	// Callback: traverse a block of code, invoking the given traverser interface as needed
	//
	// This function traverses the entire specified block of Epoch code, passing
	// information to the language extension for each operation in the block.
	//
	void __stdcall TraversalCallback(OriginalCodeHandle handle, Traverser::Interface* traversal, HandleType sessionhandle)
	{
		BindTraversal boundtraverser(traversal, sessionhandle);

		VM::Block* originalcode = reinterpret_cast<VM::Block*>(handle);		// This is safe since we issued the handle to begin with!

		VM::ScopeDescription* parentscope = originalcode->GetBoundScope()->ParentScope;
		while(parentscope)
		{
			boundtraverser.RegisterScope(parentscope, false);
			parentscope = parentscope->ParentScope;
		}

		originalcode->TraverseExternal(boundtraverser);
	}


	void __stdcall TraverseFunctionCallback(const wchar_t* functionname, Traverser::Interface* traversal, HandleType session, HandleType program)
	{
		VM::Program* theprogram = reinterpret_cast<VM::Program*>(program);
		VM::Function* targetfunction = dynamic_cast<VM::Function*>(theprogram->GetGlobalScope().GetFunction(functionname));
		if(!targetfunction)
			throw Exception("Could not find the requested function");

		BindTraversal boundtraverser(traversal, session);
		boundtraverser.TraverseFunction(functionname, targetfunction);
	}


	//
	// Convert a traverser payload wrapper structure to a VM RValue wrapper
	//
	VM::RValuePtr PayloadToRValue(const Traverser::Payload& payload)
	{
		// TODO - support additional types
		switch(payload.Type)
		{
		case VM::EpochVariableType_Integer:			return VM::RValuePtr(new VM::IntegerRValue(payload.Int32Value));
		case VM::EpochVariableType_Integer16:		return VM::RValuePtr(new VM::Integer16RValue(payload.Int16Value));
		case VM::EpochVariableType_Real:			return VM::RValuePtr(new VM::RealRValue(payload.FloatValue));
		case VM::EpochVariableType_String:			return VM::RValuePtr(new VM::StringRValue(payload.StringValue));
		case VM::EpochVariableType_Array:			return VM::RValuePtr(new VM::ArrayRValue(payload.ParameterType, payload.ParameterCount, payload.PointerValue));
		}

		throw Exception("Unsupported type, cannot convert language extension data into a VM-friendly format");
	}


	//
	// Callback: the language extension wishes to set the value of an Epoch variable
	//
	void __stdcall MarshalCallbackWrite(HandleType handle, const wchar_t* identifier, Traverser::Payload* payload)
	{
		VM::ActivatedScope* activatedscope = reinterpret_cast<VM::ActivatedScope*>(handle);						// This is safe since we issued the handle to begin with!
		VM::EpochVariableTypeID desttype = activatedscope->GetVariableType(identifier);
		activatedscope->SetVariableValue(identifier, PayloadToRValue(*payload));
	}

	//
	// Callback: the language extension wishes to retrieve the value of an Epoch variable
	//
	void __stdcall MarshalCallbackRead(HandleType activatedscopehandle, const wchar_t* identifier, Traverser::Payload* payload)
	{
		VM::ActivatedScope* activatedscope = reinterpret_cast<VM::ActivatedScope*>(activatedscopehandle);		// This is safe since we issued the handle to begin with!
		payload->Type = activatedscope->GetVariableType(identifier);

		// TODO - support additional types
		switch(payload->Type)
		{
		case VM::EpochVariableType_Integer:
			payload->SetValue(activatedscope->GetVariableValue(identifier)->CastTo<VM::IntegerRValue>().GetValue());
			break;
		case VM::EpochVariableType_Integer16:
			payload->SetValue(activatedscope->GetVariableValue(identifier)->CastTo<VM::Integer16RValue>().GetValue());
			break;
		case VM::EpochVariableType_Real:
			payload->SetValue(activatedscope->GetVariableValue(identifier)->CastTo<VM::RealRValue>().GetValue());
			break;
		case VM::EpochVariableType_String:
			payload->SetValue(activatedscope->GetVariableValue(identifier)->CastTo<VM::StringRValue>().GetValue().c_str());
			break;
		case VM::EpochVariableType_Array:
			{
				HandleType handle = activatedscope->GetVariableValue(identifier)->CastTo<VM::ArrayRValue>().GetHandle();
				size_t elementcount = activatedscope->GetVariableValue(identifier)->CastTo<VM::ArrayRValue>().GetElementCount();
				payload->SetValue(VM::ArrayVariable::GetArrayStorage(handle));
				payload->ParameterCount = elementcount;
			}
			break;

		default:
			throw Exception("Unsupported type, cannot pass a variable of this type through a language extension library");
		}
	}

	//
	// Callback: register that an error occurred during some operation in the language extension library
	//
	// This function allows us to respond appropriately to errors in the extension library,
	// without needing to worry about throwing actual exceptions across DLL boundaries.
	//
	void __stdcall ErrorCallback(const wchar_t* errormessage)
	{
		throw std::exception(narrow(errormessage).c_str());
	}

}


//
// Request the library extension to register the keywords it wishes to add to the language
//
// Note that we also take this to inform the library of where to find callback
// functionality for compiling and otherwise handling Epoch code.
//
void ExtensionDLLAccess::RegisterExtensionKeywords(ExtensionLibraryHandle token)
{
	Extensions::ExtensionInterface eif;
	eif.Register = RegistrationCallback;
	eif.RegisterControl = ControlRegistrationCallback;
	eif.Traverse = TraversalCallback;
	eif.TraverseFunction = TraverseFunctionCallback;
	eif.MarshalRead = MarshalCallbackRead;
	eif.MarshalWrite = MarshalCallbackWrite;
	eif.Error = ErrorCallback;

	DoRegistration(&eif, token);
}

//
// Request the library extension to do anything it needs to do in order to begin program execution
//
void ExtensionDLLAccess::PrepareForExecution()
{
	DoPrepare(SessionHandle);
}


void ExtensionDLLAccess::FillSerializationBuffer(wchar_t*& buffer, size_t& buffersize)
{
	DoFillSerializationBuffer(&buffer, &buffersize);
}

void ExtensionDLLAccess::FreeSerializationBuffer(wchar_t* buffer)
{
	DoFreeSerializationBuffer(buffer);
}


void ExtensionDLLAccess::LoadDataBuffer(const std::string& buffer)
{
	DoLoadDataBuffer(buffer.data(), buffer.size());
}

void ExtensionDLLAccess::PrepareCodeBlock(CodeBlockHandle handle)
{
	DoPrepareBlock(handle);
}
