//
// The Epoch Language Project
// Win32 EXE Generator
//
// Icon resource compiler
//

#include "pch.h"

#include "Resource Compiler/Resource Types/Icons.h"

#include "Linker/LinkWriter.h"


using namespace ResourceCompiler;


//
// Set of structures used to interpret and convert icon data
//

#pragma pack(push)
#pragma pack(2)

struct IconImage
{
	BITMAPINFOHEADER Header;
	RGBQUAD Colors[1];
	BYTE XOR[1];
	BYTE AND[1];
};

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
// Write an icon group record to disk
//
void IconGroupEmitter::Emit(LinkWriter& writer)
{
	writer.EmitWORD(0);						// Reserved
	writer.EmitWORD(1);						// Type (1 == icon)
	writer.EmitWORD(static_cast<WORD>(Memberships.count(ID)));

	std::pair<std::multimap<DWORD, IconEmitter*>::const_iterator, std::multimap<DWORD, IconEmitter*>::const_iterator> range = Memberships.equal_range(ID);
	for(std::multimap<DWORD, IconEmitter*>::const_iterator iter = range.first; iter != range.second; ++iter)
		iter->second->EmitGroupEntry(writer);
}

//
// Determine how much disk space the icon group entry requires
//
DWORD IconGroupEmitter::GetSize() const
{
	return static_cast<DWORD>((sizeof(WORD) * 3 + sizeof(DWORD) + 4) * Memberships.count(ID) + sizeof(WORD) * 3);
}


//
// Construct and initialize an icon emitter
//
IconEmitter::IconEmitter(const std::wstring& filename)
	: Filename(filename)
{
	std::ifstream infile(Filename.c_str(), std::ios::binary);
	if(!infile)
		throw Exception("FAILED to compile .ico resource!");

	infile.read(reinterpret_cast<Byte*>(&Directory), sizeof(Directory));

	if(Directory.Type != 0x01)
		throw Exception("FAILED to compile .ico resource, doesn't appear to be a valid icon");
}

//
// Write an icon resource to disk
//
void IconEmitter::Emit(LinkWriter& writer)
{
	// We already validated the file in the ctor so we just assume it's still fine
	std::ifstream infile(Filename.c_str(), std::ios::binary);

	for(unsigned i = 0; i < Directory.Count; ++i)
	{
		infile.seekg(Directory.Entries[i].ImageOffset);
		std::vector<Byte> innerbuffer(Directory.Entries[i].BytesInResource);
		infile.read(&innerbuffer[0], static_cast<std::streamsize>(innerbuffer.size()));
		writer.EmitBlob(&innerbuffer[0], innerbuffer.size());
	}
}

//
// Determine how much disk space the icon resource requires
//
DWORD IconEmitter::GetSize() const
{
	DWORD size = 0;
	for(unsigned i = 0; i < Directory.Count; ++i)
		size += Directory.Entries[i].BytesInResource;

	return size;
}


//
// Write icon metadata block - this is written as part of the
// icon group resource data, so we have to do it separately
// from the actual icon image data.
//
void IconEmitter::EmitGroupEntry(LinkWriter& writer)
{
	for(unsigned i = 0; i < Directory.Count; ++i)
	{
		ResourceIconDirectoryEntry entry;
		entry.Width = Directory.Entries[i].Width;
		entry.Height = Directory.Entries[i].Height;
		entry.ColorCount = Directory.Entries[i].ColorCount;
		entry.Reserved = 0x00;
		entry.Planes = Directory.Entries[i].Planes;
		entry.BitCount = Directory.Entries[i].BitCount;
		entry.BytesInResource = Directory.Entries[i].BytesInResource;
		entry.ID = i + 1;
		writer.EmitBlob(&entry, sizeof(entry));
	}
}

