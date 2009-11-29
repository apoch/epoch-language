//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Management of all registered language extensions
//

#include "pch.h"

#include "Language Extensions/ExtensionCatalog.h"
#include "Language Extensions/DLLAccess.h"


using namespace Extensions;


std::map<std::wstring, ExtensionLibraryHandle> Extensions::ExtensionKeywordMap;
std::map<std::wstring, ExtensionLibraryHandle> ExtensionLibraryDLLMap;
std::map<ExtensionLibraryHandle, ExtensionDLLAccess> ExtensionLibraryMap;


ExtensionLibraryHandle handlecounter = 0;


ExtensionLibraryHandle Extensions::RegisterExtensionLibrary(const std::wstring& libraryname)
{
	std::wstring fulldllname = libraryname + L".dll";

	std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionLibraryDLLMap.find(fulldllname);
	if(iter != ExtensionLibraryDLLMap.end())
		return iter->second;

	ExtensionLibraryHandle handle = ++handlecounter;

	ExtensionLibraryDLLMap.insert(std::make_pair(fulldllname, handle));
	ExtensionLibraryMap.insert(std::make_pair(handle, ExtensionDLLAccess(fulldllname)));

	ExtensionLibraryMap.find(handle)->second.RegisterExtensionKeywords(handle);

	return handle;
}

ExtensionLibraryHandle Extensions::GetLibraryProvidingExtension(const std::wstring& extensionname)
{
	std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionKeywordMap.find(extensionname);
	if(iter == ExtensionKeywordMap.end())
		throw Exception("The requested language extension is not provided by any currently loaded extension libraries");

	return iter->second;
}


CodeBlockHandle Extensions::BindLibraryToCode(ExtensionLibraryHandle libhandle, VM::Block* codeblock)
{
	std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.find(libhandle);
	if(iter == ExtensionLibraryMap.end())
		throw Exception("Language extension library handle is invalid; has the library been unloaded?");

	return iter->second.LoadSourceBlock(reinterpret_cast<OriginalCodeHandle>(codeblock));
}


void Extensions::ExecuteBoundCodeBlock(ExtensionLibraryHandle libhandle, CodeBlockHandle codehandle, HandleType activatedscopehandle)
{
	std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.find(libhandle);
	if(iter == ExtensionLibraryMap.end())
		throw Exception("Language extension library handle is invalid; has the library been unloaded?");

	iter->second.ExecuteSourceBlock(codehandle, activatedscopehandle);
}


void Extensions::RegisterExtensionKeyword(const std::wstring& keyword, ExtensionLibraryHandle handler)
{
	std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionKeywordMap.find(keyword);
	if(iter != ExtensionKeywordMap.end())
		throw Exception("Multiple language extensions are trying to use the same extension keyword");

	ExtensionKeywordMap.insert(std::make_pair(keyword, handler));
}

void Extensions::PrepareForExecution()
{
	for(std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.begin(); iter != ExtensionLibraryMap.end(); ++iter)
		iter->second.PrepareForExecution();
}

