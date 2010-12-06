//
// The Epoch Language Project
// Win32 EXE Generator
//
// Definitions of strings embedded in the Epoch launcher stub
//

#pragma once


#define EMBEDDED_STRING_TABLE											\
	STRING(STRINGS_FUGUEDLLNAME, L"epochvm.dll")						\
	STRING(STRINGS_EPOCHSUBSYSTEM, L"Epoch Subsystem")					\
	STRING(STRINGS_FAILEDFUGUEDLL, L"Failed to load Epoch Virtual Machine DLL; ensure that EpochVM.DLL is present.") \
	STRING(STRINGS_EXECUTEBINBUFFER, "ExecuteByteCode")					\
	STRING(STRINGS_FAILEDFUNCTIONS, L"One or more Epoch service functions could not be loaded from EpochVM.DLL; please ensure the latest version of Epoch is present.") \
	STRING(STRINGS_FAILEDEXE, L"Failed to open .EXE for reading")		\
	STRING(STRINGS_FAILMAP, L"Failed to map file to memory")			\
	STRING(STRINGS_FAILVIEW, L"Failed to map file view")				\


#define STRING(StringID, StringContent)									\
	StringID,															\

enum StringIDEnum
{
	EMBEDDED_STRING_TABLE
};

#undef STRING

