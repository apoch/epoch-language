//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Management of all registered language extensions
//

#include "pch.h"

#include "Language Extensions/ExtensionCatalog.h"
#include "Language Extensions/DLLAccess.h"

#include "Utility/Files/FilesAndPaths.h"


using namespace Extensions;


std::map<std::wstring, ExtensionLibraryHandle> Extensions::ExtensionKeywordMap;
std::map<std::wstring, ExtensionLibraryHandle> ExtensionLibraryDLLMap;
std::map<ExtensionLibraryHandle, ExtensionDLLAccess> ExtensionLibraryMap;
std::map<std::wstring, std::vector<ExtensionControlParamInfo> > Extensions::ExtensionControlParamMap;


ExtensionLibraryHandle handlecounter = 0;


//
// Load a given extension DLL and have the extension register any new keywords
//
ExtensionLibraryHandle Extensions::RegisterExtensionLibrary(const std::wstring& libraryname, VM::Program& program)
{
	std::wstring fulldllname = libraryname + L".dll";

	std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionLibraryDLLMap.find(fulldllname);
	if(iter != ExtensionLibraryDLLMap.end())
		return iter->second;

	ExtensionLibraryHandle handle = ++handlecounter;

	ExtensionLibraryDLLMap.insert(std::make_pair(fulldllname, handle));
	ExtensionLibraryMap.insert(std::make_pair(handle, ExtensionDLLAccess(fulldllname, program)));

	ExtensionLibraryMap.find(handle)->second.RegisterExtensionKeywords(handle);

	return handle;
}

//
// Given an extension keyword, locate the library providing the extension
//
ExtensionLibraryHandle Extensions::GetLibraryProvidingExtension(const std::wstring& extensionname)
{
	std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionKeywordMap.find(extensionname);
	if(iter == ExtensionKeywordMap.end())
		throw Exception("The requested language extension is not provided by any currently loaded extension libraries");

	return iter->second;
}

//
// Process the contents of an extension block
//
// When an extension keyword is used, the attached code block is passed
// through this function to the extension library so the library can do
// any processing it likes on the given code, such as compiling it into
// a different language.
//
CodeBlockHandle Extensions::BindLibraryToCode(ExtensionLibraryHandle libhandle, const std::wstring& keyword, VM::Block* codeblock)
{
	std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.find(libhandle);
	if(iter == ExtensionLibraryMap.end())
		throw Exception("Language extension library handle is invalid; has the library been unloaded?");

	return iter->second.LoadSourceBlock(keyword, reinterpret_cast<OriginalCodeHandle>(codeblock));
}

//
// Execute the processed contents of an extension block
//
// Once an extension has processed a block of code, that code can be
// invoked via this function.
//
void Extensions::ExecuteBoundCodeBlock(ExtensionLibraryHandle libhandle, CodeBlockHandle codehandle, HandleType activatedscopehandle)
{
	std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.find(libhandle);
	if(iter == ExtensionLibraryMap.end())
		throw Exception("Language extension library handle is invalid; has the library been unloaded?");

	iter->second.ExecuteSourceBlock(codehandle, activatedscopehandle);
}


void Extensions::ExecuteBoundCodeBlock(ExtensionLibraryHandle libhandle, CodeBlockHandle codehandle, HandleType activatedscopehandle, const std::vector<Traverser::Payload>& payloads)
{
	std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.find(libhandle);
	if(iter == ExtensionLibraryMap.end())
		throw Exception("Language extension library handle is invalid; has the library been unloaded?");

	iter->second.ExecuteSourceBlock(codehandle, activatedscopehandle, payloads);
}


//
// Link a given new language keyword to the library that handles it
//
void Extensions::RegisterExtensionKeyword(const std::wstring& keyword, ExtensionLibraryHandle handler)
{
	std::map<std::wstring, ExtensionLibraryHandle>::const_iterator iter = ExtensionKeywordMap.find(keyword);
	if(iter != ExtensionKeywordMap.end())
		throw Exception("Multiple language extensions are trying to use the same extension keyword");

	ExtensionKeywordMap.insert(std::make_pair(keyword, handler));
}


void Extensions::RegisterExtensionControl(const std::wstring& keyword, ExtensionLibraryHandle token, size_t numparams, ExtensionControlParamInfo* params)
{
	RegisterExtensionKeyword(keyword, token);

	std::vector<ExtensionControlParamInfo> paramvector;
	paramvector.reserve(numparams);

	for(unsigned i = 0; i < numparams; ++i)
		paramvector.push_back(params[i]);

	ExtensionControlParamMap.insert(std::make_pair(keyword, paramvector));
}


//
// Signal all extensions to do any required preparatory work prior to executing the program
//
void Extensions::PrepareForExecution()
{
	for(std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.begin(); iter != ExtensionLibraryMap.end(); ++iter)
		iter->second.PrepareForExecution();
}

const std::wstring& Extensions::GetDLLFileOfLibrary(ExtensionLibraryHandle handle)
{
	std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::iterator iter = ExtensionLibraryMap.find(handle);
	if(iter == ExtensionLibraryMap.end())
		throw Exception("Language extension library handle is invalid; has the library been unloaded?");

	return iter->second.GetDLLFileName();
}

std::set<std::wstring> Extensions::GetAllExtensionDLLs()
{
	std::set<std::wstring> ret;
	for(std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::const_iterator iter = ExtensionLibraryMap.begin(); iter != ExtensionLibraryMap.end(); ++iter)
		ret.insert(StripExtension(iter->second.GetDLLFileName()));
	return ret;
}

const std::vector<ExtensionControlParamInfo>& Extensions::GetParamsForControl(const std::wstring& keyword)
{
	std::map<std::wstring, std::vector<ExtensionControlParamInfo> >::const_iterator iter = ExtensionControlParamMap.find(keyword);
	if(iter == ExtensionControlParamMap.end())
		throw Exception("No language extension is loaded which handles this keyword");

	return iter->second;
}

bool Extensions::ExtensionIsAvailableForExecution(ExtensionLibraryHandle handle)
{
	std::map<ExtensionLibraryHandle, ExtensionDLLAccess>::const_iterator iter = ExtensionLibraryMap.find(handle);
	if(iter == ExtensionLibraryMap.end())
		return false;

	return iter->second.IsAvailableForExecution();
}

