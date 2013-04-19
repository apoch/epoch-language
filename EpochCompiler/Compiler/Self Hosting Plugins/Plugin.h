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
#include <string>
#include <map>


class CompilerPluginManager
{
// Plugin registration
public:
	void RegisterPluginFunction(const std::wstring& functionname, void* code);

// Internal state
private:
	std::map<std::wstring, void*> PluginRegistrationTable;
};

