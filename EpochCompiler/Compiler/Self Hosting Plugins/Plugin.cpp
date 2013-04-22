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

#include "pch.h"

#include "Compiler/Self Hosting Plugins/Plugin.h"


CompilerPluginManager Plugins;



void CompilerPluginManager::RegisterPluginFunction(const std::wstring& functionname, void* code)
{
	PluginRegistrationTable.insert(std::make_pair(functionname, code));
}


bool CompilerPluginManager::IsPluginFunctionProvided(const std::wstring& functionname)
{
	std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
	return (iter != PluginRegistrationTable.end());
}


void CompilerPluginManager::InvokeVoidPluginFunction(const std::wstring& functionname)
{
	std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
	if(iter == PluginRegistrationTable.end())
		throw FatalException("No plugin loaded provides the requested function");

	typedef void (STDCALL *funcptr)();
	funcptr p = reinterpret_cast<funcptr>(iter->second);

	p();
}

Integer32 CompilerPluginManager::InvokeIntegerPluginFunction(const std::wstring& functionname)
{
	std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
	if(iter == PluginRegistrationTable.end())
		throw FatalException("No plugin loaded provides the requested function");

	typedef Integer32 (STDCALL *funcptr)();
	funcptr p = reinterpret_cast<funcptr>(iter->second);

	return p();
}


const Byte* CompilerPluginManager::InvokeBytePointerPluginFunction(const std::wstring& functionname)
{
	std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
	if(iter == PluginRegistrationTable.end())
		throw FatalException("No plugin loaded provides the requested function");

	typedef const Byte* (STDCALL *funcptr)();
	funcptr p = reinterpret_cast<funcptr>(iter->second);

	return p();
}


void CompilerPluginManager::Clear()
{
	PluginRegistrationTable.clear();
}

