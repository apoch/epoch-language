//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// DLL import/export shortcut macros
//

#pragma once

#ifdef EPOCHVM_EXPORTS
#define EPOCHVM __declspec(dllexport)
#else
#define EPOCHVM __declspec(dllimport)
#endif
