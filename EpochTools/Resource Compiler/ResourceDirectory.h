//
// The Epoch Language Project
// Win32 EXE Generator
//
// Resource Directory management classes
//

#pragma once


// Forward declarations
class LinkWriter;


// Dependencies
#include <list>
#include <map>


namespace ResourceCompiler
{

	// Forward declarations
	class ResourceEmitter;

	// Implementation hiding
	namespace
	{
		class DirectoryHeader;
	}

	//
	// General resource directory wrapper
	//
	// Maintains the resource directory tree data structure and performs some assorted
	// housekeeping for ensuring that offset and size data is maintained correctly for
	// each element of the data structure.
	//
	class ResourceDirectory
	{
	// Construction and destruction
	public:
		ResourceDirectory();
		~ResourceDirectory();

	// Resource management interface
	public:
		void AddResource(DWORD type, DWORD id, DWORD language, ResourceEmitter* emitter);
		void ComputeOffsets();

	// Linker interface
	public:
		void Emit(LinkWriter& writer, DWORD virtualbaseaddress) const;
		DWORD GetSize() const;

	// Internal helpers
	private:
		struct ResourceRecord
		{
			ResourceRecord(DWORD type, DWORD id, DWORD language, ResourceEmitter* emitter)
				: Type(type), ID(id), Language(language), Emitter(emitter)
			{ }

			DWORD Type;
			DWORD ID;
			DWORD Language;
			ResourceEmitter* Emitter;
		};

	// Internal tracking
	private:
		std::list<ResourceRecord> ResourceRecords;

		DirectoryHeader* RootTier;

		DWORD DirectorySize;
		DWORD ResourceSize;

		bool OffsetsComputed;
	};
}


