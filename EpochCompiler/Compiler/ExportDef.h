//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// DLL import/export shortcut macros
//

#pragma once

#ifdef EPOCHCOMPILER_EXPORTS
#define EPOCHCOMPILER __declspec(dllexport)
#else
#define EPOCHCOMPILER __declspec(dllimport)
#endif
