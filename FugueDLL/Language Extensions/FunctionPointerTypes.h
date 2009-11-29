//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Common function pointer types and interface structures for language extensions
//

#pragma once


// Dependencies
#include "Language Extensions/HandleTypes.h"


// Forward declarations
namespace Traverser
{
	struct Interface;
	struct Payload;
}


namespace Extensions
{

	typedef void (__stdcall *RegistrationCallbackPtr)(ExtensionLibraryHandle token, const wchar_t* keyword);
	typedef void (__stdcall *TraversalCallbackPtr)(OriginalCodeHandle handle, Traverser::Interface* traversal, HandleType session);
	typedef void (__stdcall *MarshalCallbackReadPtr)(HandleType activatedscopehandle, const wchar_t* identifier, Traverser::Payload* payload);
	typedef void (__stdcall *MarshalCallbackWritePtr)(HandleType activatedscopehandle, const wchar_t* identifier, Traverser::Payload* payload);
	typedef void (__stdcall *ErrorCallbackPtr)(const wchar_t* errorstring);


	//
	// POD container of function pointers for convenience
	//
	struct ExtensionInterface
	{
		RegistrationCallbackPtr Register;
		TraversalCallbackPtr Traverse;
		MarshalCallbackReadPtr MarshalRead;
		MarshalCallbackWritePtr MarshalWrite;
		ErrorCallbackPtr Error;
	};

}

