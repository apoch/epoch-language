//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Functions exported by the interop DLL
//

#pragma once


// Dependencies
#include "Traverser/TraversalInterface.h"


// Functions that make sense to know about throughout the compiler code
void __stdcall NodeEntryCallback(Traverser::TraversalSessionHandle sessionhandle);
void __stdcall NodeExitCallback(Traverser::TraversalSessionHandle sessionhandle);
void __stdcall LeafCallback(Traverser::TraversalSessionHandle sessionhandle, const wchar_t* token, const Traverser::Payload* payload);
void __stdcall ScopeCallback(Traverser::TraversalSessionHandle sessionhandle, bool toplevel, size_t numcontents, const Traverser::ScopeContents* contents);
