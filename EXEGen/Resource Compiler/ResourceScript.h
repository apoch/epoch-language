//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper objects for managing resource script files
//

#pragma once


namespace ResourceCompiler
{

	// Forward declarations
	class ResourceDirectory;
	class IconEmitter;


	class ResourceScript
	{
	// Construction
	public:
		explicit ResourceScript(const std::wstring& filename);
		~ResourceScript();

	// Resource loading interface
	public:
		void AddResourcesToDirectory(ResourceDirectory& directory);

	// Internal helpers
	private:
		void LoadResourceIntoDirectory(DWORD type, size_t offset, ResourceDirectory& directory);

	// Internal tracking
	private:
		std::wstring Filename;
		std::multimap<DWORD, size_t> ResourceOffsets;
		std::multimap<DWORD, IconEmitter*> IconEmitters;
	};

}

