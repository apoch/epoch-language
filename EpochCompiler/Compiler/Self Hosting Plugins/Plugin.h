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
	void Clear();

// Plugin checking
public:
	bool IsPluginFunctionProvided(const std::wstring& functionname);

// Plugin invocation
public:
	void InvokeVoidPluginFunction(const std::wstring& functionname);

	template<typename T1>
	void InvokeVoidPluginFunction(const std::wstring& functionname, T1 param)
	{
		std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
		if(iter == PluginRegistrationTable.end())
			throw FatalException("No plugin loaded provides the requested function");

		typedef void (STDCALL *funcptr)(T1);
		funcptr p = reinterpret_cast<funcptr>(iter->second);

		p(param);
	}

	template<typename T1, typename T2>
	void InvokeVoidPluginFunction(const std::wstring& functionname, T1 param1, T2 param2)
	{
		std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
		if(iter == PluginRegistrationTable.end())
			throw FatalException("No plugin loaded provides the requested function");

		typedef void (STDCALL *funcptr)(T1, T2);
		funcptr p = reinterpret_cast<funcptr>(iter->second);

		p(param1, param2);
	}

	template<typename T1, typename T2, typename T3>
	void InvokeVoidPluginFunction(const std::wstring& functionname, T1 param1, T2 param2, T3 param3)
	{
		std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
		if(iter == PluginRegistrationTable.end())
			throw FatalException("No plugin loaded provides the requested function");

		typedef void (STDCALL *funcptr)(T1, T2, T3);
		funcptr p = reinterpret_cast<funcptr>(iter->second);

		p(param1, param2, param3);
	}


	template<typename T1, typename T2, typename T3, typename T4>
	void InvokeVoidPluginFunction(const std::wstring& functionname, T1 param1, T2 param2, T3 param3, T4 param4)
	{
		std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
		if(iter == PluginRegistrationTable.end())
			throw FatalException("No plugin loaded provides the requested function");

		typedef void (STDCALL *funcptr)(T1, T2, T3, T4);
		funcptr p = reinterpret_cast<funcptr>(iter->second);

		p(param1, param2, param3, param4);
	}


	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	void InvokeVoidPluginFunction(const std::wstring& functionname, T1 param1, T2 param2, T3 param3, T4 param4, T5 param5)
	{
		std::map<std::wstring, void*>::const_iterator iter = PluginRegistrationTable.find(functionname);
		if(iter == PluginRegistrationTable.end())
			throw FatalException("No plugin loaded provides the requested function");

		typedef void (STDCALL *funcptr)(T1, T2, T3, T4, T5);
		funcptr p = reinterpret_cast<funcptr>(iter->second);

		p(param1, param2, param3, param4, param5);
	}

	Integer32 InvokeIntegerPluginFunction(const std::wstring& functionname);

	const Byte* InvokeBytePointerPluginFunction(const std::wstring& functionname);

// Internal state
private:
	std::map<std::wstring, void*> PluginRegistrationTable;
};



extern CompilerPluginManager Plugins;

