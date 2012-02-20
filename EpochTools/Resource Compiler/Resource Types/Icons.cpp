//
// The Epoch Language Project
// Win32 EXE Generator
//
// Icon resource compiler
//

#include "pch.h"

#include "Resource Compiler/Resource Types/Icons.h"
#include "Resource Compiler/ResourceDirectory.h"
#include "Resource Compiler/ResourceTypes.h"

#include "Linker/LinkWriter.h"

#include "Utility/Types/IntegerTypes.h"

#include <fstream>


using namespace ResourceCompiler;


//
// Set of structures used to interpret and convert icon data
//

#pragma pack(push)
#pragma pack(1)

struct ResourceIconDirectoryEntry
{
	BYTE Width;
	BYTE Height;
	BYTE ColorCount;
	BYTE Reserved;
	WORD Planes;
	WORD BitCount;
	DWORD BytesInResource;
	WORD ID;
};

#pragma pack(pop)


//
// Construct and initialize an icon group membership helper
//
IconGroupMemberships::IconGroupMemberships()
	: CurrentID(0)
{
}

//
// Allocate a new icon ID and associate it with the given group
//
DWORD IconGroupMemberships::AllocateID(DWORD group, size_t imageindex, const IconUnpacker* unpacker)
{
	++CurrentID;
	Memberships.insert(std::make_pair(group, CurrentID));
	Indexes.insert(std::make_pair(CurrentID, imageindex));
	Unpackers.insert(std::make_pair(CurrentID, unpacker));
	return CurrentID;
}

//
// Get the number of members in a given group
//
size_t IconGroupMemberships::GetMembershipCount(DWORD group) const
{
	return Memberships.count(group);
}

//
// Emit the membership data for a given group
//
void IconGroupMemberships::EmitMembershipsForGroup(DWORD group, LinkWriter& writer) const
{
	std::pair<std::multimap<DWORD, DWORD>::const_iterator, std::multimap<DWORD, DWORD>::const_iterator> range = Memberships.equal_range(group);
	for(std::multimap<DWORD, DWORD>::const_iterator iter = range.first; iter != range.second; ++iter)
	{
		std::map<DWORD, size_t>::const_iterator indexiter = Indexes.find(iter->second);
		if(indexiter == Indexes.end())
			throw Exception("Internal failure in IconGroupMemberships - no icon index found for the given icon ID");

		size_t index = indexiter->second;

		std::map<DWORD, const IconUnpacker*>::const_iterator unpackeriter = Unpackers.find(iter->second);
		if(unpackeriter == Unpackers.end())
			throw Exception("Internal failure in IconGroupMemberships - no unpacker found for the given icon ID");

		const IconUnpacker& unpacker = *(unpackeriter->second);

		ResourceIconDirectoryEntry entry;
		entry.Width = unpacker.Directory[index].Width;
		entry.Height = unpacker.Directory[index].Height;
		entry.ColorCount = unpacker.Directory[index].ColorCount;
		entry.Reserved = 0x00;
		entry.Planes = unpacker.Directory[index].Planes;
		entry.BitCount = unpacker.Directory[index].BitCount;
		entry.BytesInResource = unpacker.Directory[index].BytesInResource;
		entry.ID = static_cast<WORD>(iter->second);
		writer.EmitBlob(&entry, sizeof(entry));
	}
}


//
// Construct and initialize an icon unpacking helper
//
IconUnpacker::IconUnpacker(const std::wstring& filename, DWORD group, DWORD language)
	: FileName(filename),
	  Group(group),
	  Language(language)
{
	std::ifstream infile(filename.c_str(), std::ios::binary);
	if(!infile)
		throw Exception("FAILED to compile .ico resource!");

	infile.read(reinterpret_cast<Byte*>(&DirectoryHeader), sizeof(DirectoryHeader));

	if(DirectoryHeader.Type != 0x01)
		throw Exception("FAILED to compile .ico resource, doesn't appear to be a valid icon");

	if(DirectoryHeader.Count == 0)
		throw Exception("FAILED to compile .ico resource, doesn't appear to contain any images");

	Directory.resize(DirectoryHeader.Count);
	infile.read(reinterpret_cast<Byte*>(&Directory[0]), static_cast<std::streamsize>(sizeof(IconDirectoryEntry) * Directory.size()));
}

//
// Unpack an icon and convert it into the emitters used in a resource directory wrapper
//
void IconUnpacker::CreateResources(ResourceDirectory& directory, IconGroupMemberships& memberships) const
{
	for(size_t i = 0; i < Directory.size(); ++i)
	{
		std::auto_ptr<IconEmitter> emitter(new IconEmitter(FileName, Directory[i].ImageOffset, Directory[i].BytesInResource));
		directory.AddResource(RESTYPE_ICON, memberships.AllocateID(Group, i, this), Language, emitter.release());
	}
}


//
// Write an icon group record to disk
//
void IconGroupEmitter::Emit(LinkWriter& writer) const
{
	writer.EmitWORD(0);						// Reserved
	writer.EmitWORD(1);						// Type (1 == icon)
	writer.EmitWORD(static_cast<WORD>(Memberships.GetMembershipCount(ID)));

	Memberships.EmitMembershipsForGroup(ID, writer);
}

//
// Determine how much disk space the icon group entry requires
//
DWORD IconGroupEmitter::GetSize() const
{
	return static_cast<DWORD>(sizeof(ResourceIconDirectoryEntry) * Memberships.GetMembershipCount(ID) + sizeof(WORD) * 3);
}


//
// Construct and initialize an icon emitter
//
IconEmitter::IconEmitter(const std::wstring& filename, unsigned offset, unsigned size)
	: Filename(filename),
	  ImageOffset(offset),
	  ImageSize(size)
{
}

//
// Write an icon resource to disk
//
void IconEmitter::Emit(LinkWriter& writer) const
{
	// We already validated the file in the ctor so we just assume it's still fine
	std::ifstream infile(Filename.c_str(), std::ios::binary);

	infile.seekg(ImageOffset);
	std::vector<Byte> innerbuffer(ImageSize);
	infile.read(&innerbuffer[0], static_cast<std::streamsize>(innerbuffer.size()));
	writer.EmitBlob(&innerbuffer[0], innerbuffer.size());
}

//
// Determine how much disk space the icon resource requires
//
DWORD IconEmitter::GetSize() const
{
	return ImageSize;
}


