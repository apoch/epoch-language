//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper objects for managing resource script files
//

#pragma once


// Dependencies
#include "Resource Compiler/Resource Types/Icons.h"

#include <list>
#include <map>
#include <set>


namespace ResourceCompiler
{

	// Forward declarations
	class ResourceDirectory;
	class IconUnpacker;


	class ResourceScript
	{
	// Construction and destruction
	public:
		explicit ResourceScript(const std::list<std::wstring>& resourcefiles);
		~ResourceScript();

	// Resource loading interface
	public:
		void AddResourcesToDirectory(ResourceDirectory& directory);

	// Internal helpers
	private:
		void ProcessScriptFile(const std::wstring& filename);
		void LoadResourceIntoDirectory(DWORD type, const std::wstring& filename, size_t offset, ResourceDirectory& directory);

	// Internal tracking
	private:
		std::list<std::wstring> Filenames;

		struct OffsetInfo
		{
			std::wstring Filename;
			size_t Offset;

			OffsetInfo(const std::wstring& filename, size_t offset)
				: Filename(filename), Offset(offset)
			{ }
		};

		std::multimap<DWORD, OffsetInfo> ResourceOffsets;
		std::set<IconUnpacker*> IconUnpackers;
		IconGroupMemberships GroupMemberships;
	};

}

