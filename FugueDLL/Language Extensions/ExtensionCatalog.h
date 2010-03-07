//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Management of all registered language extensions
//

#pragma once


// Dependencies
#include "Language Extensions/HandleTypes.h"
#include "Utility/Types/EpochTypeIDs.h"
#include "Traverser/TraversalInterface.h"


// Forward declarations
namespace VM
{
	class Block;
	class Program;
}

namespace Serialization
{
	class SerializationTraverser;
}


namespace Extensions
{

	// Forward declarations
	struct ExtensionControlParamInfo;

	void PrepareForExecution();

	bool ExtensionIsAvailableForExecution(ExtensionLibraryHandle handle);

	ExtensionLibraryHandle RegisterExtensionLibrary(const std::wstring& libraryname, VM::Program& program, bool startsession);
	ExtensionLibraryHandle GetLibraryProvidingExtension(const std::wstring& extensionname);
	const std::wstring& GetDLLFileOfLibrary(ExtensionLibraryHandle handle);
	std::set<std::wstring> GetAllExtensionDLLs();

	void RegisterExtensionKeyword(const std::wstring& keyword, ExtensionLibraryHandle handler);
	void RegisterExtensionControl(const std::wstring& keyword, ExtensionLibraryHandle token, size_t numparams, ExtensionControlParamInfo* params);

	CodeBlockHandle BindLibraryToCode(ExtensionLibraryHandle libhandle, const std::wstring& keyword, VM::Block* codeblock);
	void ExecuteBoundCodeBlock(ExtensionLibraryHandle libhandle, CodeBlockHandle codehandle, HandleType activatedscopehandle);
	void ExecuteBoundCodeBlock(ExtensionLibraryHandle libhandle, CodeBlockHandle codehandle, HandleType activatedscopehandle, const std::vector<Traverser::Payload>& payloads);

	const std::vector<ExtensionControlParamInfo>& GetParamsForControl(const std::wstring& keyword);

	void LoadDataBuffer(const std::string& libraryname, const std::string& datablock);

	void PrepareCodeBlockForExecution(ExtensionLibraryHandle libhandle, CodeBlockHandle codehandle);


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


	template <typename EnumCallback>
	void EnumerateExtensionControls(EnumCallback callback)
	{
		for(std::map<std::wstring, std::vector<ExtensionControlParamInfo> >::const_iterator iter = ExtensionControlParamMap.begin(); iter != ExtensionControlParamMap.end(); ++iter)
			callback(iter->first, iter->second);
	}


	template <typename TraverserT>
	void TraverseExtensions(TraverserT& traverser)
	{
		// Do nothing, by default
	}

	template <>
	void TraverseExtensions<Serialization::SerializationTraverser>(Serialization::SerializationTraverser& traverser);


	extern std::map<std::wstring, ExtensionLibraryHandle> ExtensionKeywordMap;
	extern std::map<std::wstring, std::vector<ExtensionControlParamInfo> > ExtensionControlParamMap;
}

