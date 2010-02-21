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
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Variables/ArrayVariable.h"

#include "Utility/Strings.h"


using namespace Extensions;



//
// Construct the access wrapper and initialize the DLL bindings
//
ExtensionDLLAccess::ExtensionDLLAccess(const std::wstring& dllname)
	: SessionHandle(0),
	  DLLName(dllname)
{
	// Load the DLL
	HINSTANCE DLLHandle = ::LoadLibrary(dllname.c_str());
	if(!DLLHandle)
		throw Exception("A language extension DLL was requested, but the DLL was either not found or reported some error during initialization");

	// Obtain interface into DLL
	DoRegistration = reinterpret_cast<RegistrationPtr>(::GetProcAddress(DLLHandle, "Register"));
	DoLoadSource = reinterpret_cast<LoadSourceBlockPtr>(::GetProcAddress(DLLHandle, "LoadSourceBlock"));
	DoExecuteSource = reinterpret_cast<ExecuteSourceBlockPtr>(::GetProcAddress(DLLHandle, "ExecuteSourceBlock"));
	DoPrepare = reinterpret_cast<PreparePtr>(::GetProcAddress(DLLHandle, "CommitCompilation"));
	DoStartSession = reinterpret_cast<StartCompileSessionPtr>(::GetProcAddress(DLLHandle, "StartNewProgramCompilation"));

	// Validate interface to be sure
	if(!DoRegistration || !DoLoadSource || !DoExecuteSource || !DoPrepare || !DoStartSession)
		throw Exception("One or more Epoch service functions could not be loaded from the requested language extension DLL");

	SessionHandle = DoStartSession();
}

//
// Destruct the access wrapper and free the library
//
ExtensionDLLAccess::~ExtensionDLLAccess()
{
	::FreeLibrary(DLLHandle);
}


//
// Create a block of code in the language extension, given a block of raw Epoch code
//
CodeBlockHandle ExtensionDLLAccess::LoadSourceBlock(OriginalCodeHandle handle)
{
	return DoLoadSource(SessionHandle, handle);
}

//
// Invoke the code generated for the specified language extension code block
//
void ExtensionDLLAccess::ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle)
{
	return DoExecuteSource(handle, activatedscopehandle);
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

	//
	// Callback: traverse a block of code, invoking the given traverser interface as needed
	//
	// This function traverses the entire specified block of Epoch code, passing
	// information to the language extension for each operation in the block.
	//
	void __stdcall TraversalCallback(OriginalCodeHandle handle, Traverser::Interface* traversal, HandleType sessionhandle)
	{
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

			void RegisterScope(const VM::ScopeDescription* description)
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
						content.ContainedSize = description->GetArraySize(*iter);
					}

					contents.push_back(content);
				}

				Traversal->ScopeTraversalCallback(SessionHandle, ActiveScopes.size() == 1, contents.size(), contents.size() ? &(contents[0]) : NULL);

				std::set<const VM::ScopeDescription*> ghostscopes = description->GetAllGhostScopes();
				for(std::set<const VM::ScopeDescription*>::const_iterator iter = ghostscopes.begin(); iter != ghostscopes.end(); ++iter)
					RegisterScope(*iter);

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

		BindTraversal boundtraverser(traversal, sessionhandle);

		VM::Block* originalcode = reinterpret_cast<VM::Block*>(handle);		// This is safe since we issued the handle to begin with!

		VM::ScopeDescription* parentscope = originalcode->GetBoundScope()->ParentScope;
		while(parentscope)
		{
			boundtraverser.RegisterScope(parentscope);
			parentscope = parentscope->ParentScope;
		}

		originalcode->TraverseExternal(boundtraverser);
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
		case VM::EpochVariableType_Real:			return VM::RValuePtr(new VM::RealRValue(payload.FloatValue));
		case VM::EpochVariableType_String:			return VM::RValuePtr(new VM::StringRValue(payload.StringValue));
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
		if(desttype == VM::EpochVariableType_Array)
		{
			VM::EpochVariableTypeID elementtype = activatedscope->GetOriginalDescription().GetArrayType(identifier);
			size_t elementcount = activatedscope->GetOriginalDescription().GetArraySize(identifier);
			activatedscope->SetVariableValue(identifier, VM::RValuePtr(new VM::ArrayRValue(elementtype, elementcount, payload->PointerValue)));
		}
		else
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
		case VM::EpochVariableType_Real:
			payload->SetValue(activatedscope->GetVariableValue(identifier)->CastTo<VM::RealRValue>().GetValue());
			break;
		case VM::EpochVariableType_String:
			payload->SetValue(activatedscope->GetVariableValue(identifier)->CastTo<VM::StringRValue>().GetValue().c_str());
			break;
		case VM::EpochVariableType_Array:
			payload->Type = VM::EpochVariableType_Array;
			payload->ParameterCount = activatedscope->GetOriginalDescription().GetArraySize(identifier);
			payload->PointerValue = activatedscope->GetVariableRef<VM::ArrayVariable>(identifier).GetArrayElementStorage();
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
// Note that we also take this opportunity to inform the library of where to find callback
// functionality for compiling and otherwise handling Epoch code.
//
void ExtensionDLLAccess::RegisterExtensionKeywords(ExtensionLibraryHandle token)
{
	Extensions::ExtensionInterface eif;
	eif.Register = RegistrationCallback;
	eif.Traverse = TraversalCallback;
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
