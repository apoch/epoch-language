//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Management of all registered language extensions
//

#pragma once


// Dependencies
#include "Language Extensions/HandleTypes.h"


// Forward declarations
namespace VM
{
	class Block;
	class Program;
}


namespace Extensions
{

	void PrepareForExecution();


	ExtensionLibraryHandle RegisterExtensionLibrary(const std::wstring& libraryname, VM::Program* program);
	ExtensionLibraryHandle GetLibraryProvidingExtension(const std::wstring& extensionname);
	const std::wstring& GetDLLFileOfLibrary(ExtensionLibraryHandle handle);
	std::set<std::wstring> GetAllExtensionDLLs();

	void RegisterExtensionKeyword(const std::wstring& keyword, ExtensionLibraryHandle handler);

	CodeBlockHandle BindLibraryToCode(ExtensionLibraryHandle libhandle, VM::Block* codeblock);
	void ExecuteBoundCodeBlock(ExtensionLibraryHandle libhandle, CodeBlockHandle codehandle, HandleType activatedscopehandle);


	template <typename EnumCallback>
	void EnumerateExtensionKeywords(EnumCallback callback)
	{
		for(std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionKeywordMap.begin(); iter != ExtensionKeywordMap.end(); ++iter)
			callback(iter->first);
	}

	template <typename EnumCallback>
	void EnumerateExtensionKeywords(EnumCallback callback, ExtensionLibraryHandle handle)
	{
		for(std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionKeywordMap.begin(); iter != ExtensionKeywordMap.end(); ++iter)
		{
			if(iter->second == handle)
				callback(iter->first);
		}
	}


	extern std::map<std::wstring, ExtensionLibraryHandle> ExtensionKeywordMap;

}

