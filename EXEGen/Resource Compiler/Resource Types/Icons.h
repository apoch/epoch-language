//
// The Epoch Language Project
// Win32 EXE Generator
//
// Icon resource compiler
//

#pragma once

// Forward declarations
class LinkWriter;

// Dependencies
#include "Resource Compiler/ResourceEmitter.h"


namespace ResourceCompiler
{

#pragma pack(push)
#pragma pack(2)

	struct IconDirectoryEntry
	{
		BYTE Width;
		BYTE Height;
		BYTE ColorCount;
		BYTE Reserved;
		WORD Planes;
		WORD BitCount;
		DWORD BytesInResource;
		DWORD ImageOffset;
	};

	struct IconDirectory
	{
		WORD Reserved;
		WORD Type;
		WORD Count;
		IconDirectoryEntry Entries[1];
	};

#pragma pack(pop)


	class IconEmitter : public ResourceEmitter
	{
	// Construction
	public:
		explicit IconEmitter(const std::wstring& filename);

	// Resource emitter interface
	public:
		virtual void Emit(LinkWriter& writer);
		virtual DWORD GetSize() const;

	// Additional emission helpers
	public:
		void EmitGroupEntry(LinkWriter& writer);

	// Internal tracking
	private:
		std::wstring Filename;
		IconDirectory Directory;
	};


	class IconGroupEmitter : public ResourceEmitter
	{
	// Construction
	public:
		IconGroupEmitter(DWORD id, std::multimap<DWORD, IconEmitter*>& groupmemberships)
			: ID(id), Memberships(groupmemberships)
		{ }

	// Resource emitter interface
	public:
		virtual void Emit(LinkWriter& writer);
		virtual DWORD GetSize() const;

	// Internal tracking
	private:
		DWORD ID;
		std::multimap<DWORD, IconEmitter*>& Memberships;
	};
}
