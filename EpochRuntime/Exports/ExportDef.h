//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// DLL import/export shortcut macros
//

#pragma once

#ifdef BOOST_WINDOWS
#ifdef EPOCHRUNTIME_EXPORTS
#define EPOCHRUNTIME __declspec(dllexport)
#else
#define EPOCHRUNTIME __declspec(dllimport)
#endif
#else
#define EPOCHRUNTIME
#endif
