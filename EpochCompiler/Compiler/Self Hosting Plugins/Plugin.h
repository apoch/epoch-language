//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for loading plugins written in Epoch
//
// The goal of plugins is to support a gradual transition towards
// self-hosting for the Epoch compiler. Various components of the
// C++-hosted compiler can be transitioned into Epoch versions as
// implementations become available; the plugin interface acts as
// an interop layer between the C++ code and the Epoch code.
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"

#include <string>
#include <map>


class CompilerPluginManager
{
// Plugin registration
public:
	void RegisterPluginFunction(const std::wstring& functionname, void* code);

// Plugin checking
public:
	bool IsPluginFunctionProvided(const std::wstring& functionname);

// Plugin invocation
public:
	void InvokeVoidPluginFunction(const std::wstring& functionname);
	void InvokeVoidPluginFunction(const std::wstring& functionname, Byte param);
	void InvokeVoidPluginFunction(const std::wstring& functionname, const Byte* param1, size_t param2);

	Integer32 InvokeIntegerPluginFunction(const std::wstring& functionname);

	const Byte* InvokeBytePointerPluginFunction(const std::wstring& functionname);

// Internal state
private:
	std::map<std::wstring, void*> PluginRegistrationTable;
};



extern CompilerPluginManager Plugins;

