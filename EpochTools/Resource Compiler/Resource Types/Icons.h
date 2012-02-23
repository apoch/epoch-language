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
#include <map>
#include <vector>


namespace ResourceCompiler
{

	// Forward declarations
	class ResourceDirectory;
	class IconUnpacker;


	//
	// Class for managing the memberships of icons in icon groups
	//
	class IconGroupMemberships
	{
	// Construction
	public:
		IconGroupMemberships();

	// Membership interface
	public:
		DWORD AllocateID(DWORD group, size_t imageindex, const IconUnpacker* unpacker);
		size_t GetMembershipCount(DWORD group) const;

	// Resource emitter functions
	public:
		void EmitMembershipsForGroup(DWORD group, LinkWriter& writer) const;

	// Internal tracking
	private:
		std::multimap<DWORD, DWORD> Memberships;
		std::map<DWORD, size_t> Indexes;
		std::map<DWORD, const IconUnpacker*> Unpackers;
		DWORD CurrentID;
	};


	//
	// Class for unpacking an .ICO file into a set of resource directory entries
	//
	class IconUnpacker
	{
	// Construction
	public:
		IconUnpacker(const std::wstring& filename, DWORD group, DWORD language);

	// Unpacking into a resource directory
	public:
		void CreateResources(ResourceDirectory& directory, IconGroupMemberships& memberships) const;

	// Internal helpers
	private:

#pragma pack(push)
#pragma pack(1)

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

		struct IconDirectoryHeader
		{
			WORD Reserved;
			WORD Type;
			WORD Count;
		};

#pragma pack(pop)

	// Internal tracking
	private:
		std::wstring FileName;
		DWORD Group;
		DWORD Language;

		IconDirectoryHeader DirectoryHeader;
		std::vector<IconDirectoryEntry> Directory;

		friend class IconGroupMemberships;
	};


	class IconEmitter : public ResourceEmitter
	{
	// Construction
	public:
		IconEmitter(const std::wstring& filename, unsigned offset, unsigned size);

	// Resource emitter interface
	public:
		virtual void Emit(LinkWriter& writer) const;
		virtual DWORD GetSize() const;

	// Internal tracking
	private:
		std::wstring Filename;
		unsigned ImageOffset;
		unsigned ImageSize;
	};


	class IconGroupEmitter : public ResourceEmitter
	{
	// Construction
	public:
		IconGroupEmitter(DWORD id, IconGroupMemberships& groupmemberships)
			: ID(id), Memberships(groupmemberships)
		{ }

	// Resource emitter interface
	public:
		virtual void Emit(LinkWriter& writer) const;
		virtual DWORD GetSize() const;

	// Internal tracking
	private:
		DWORD ID;
		IconGroupMemberships& Memberships;
	};
}
