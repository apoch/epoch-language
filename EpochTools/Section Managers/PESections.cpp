//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for working with Portable Executable file sections
//

#include "pch.h"
#include "PESections.h"
#include "Linker/LinkWriter.h"

#include <iostream>


//
// Retrieve the file offset of the end of the last registered section.
// This is used as the start offset of the next section.
//
DWORD PESectionManager::GetEndOfLastSection() const
{
	if(SectionInfoTable.empty())
		return 0;

	return SectionInfoTable.back().Location + SectionInfoTable.back().Size;
}

//
// Generate any information needed to fill in the file section
//
void PESectionManager::Generate(Linker& linker)
{
	std::wcout << L"Generating section headers... OK\n";
}


//
// Write final section table to the executable
//
void PESectionManager::Emit(Linker& linker, LinkWriter& writer) const
{
	std::wcout << L"Writing section headers... ";

	for(std::list<PESectionInfo>::const_iterator iter = SectionInfoTable.begin(); iter != SectionInfoTable.end(); ++iter)
	{
		IMAGE_SECTION_HEADER sectionheader;
		::ZeroMemory(&sectionheader, sizeof(sectionheader));
		memcpy(sectionheader.Name, iter->SectionName, std::min(static_cast<size_t>(IMAGE_SIZEOF_SHORT_NAME), strlen(iter->SectionName)));
		sectionheader.Misc.VirtualSize = iter->VirtualSize;
		sectionheader.VirtualAddress = iter->VirtualLocation;
		sectionheader.SizeOfRawData = iter->Size;
		sectionheader.PointerToRawData = iter->Location;
		sectionheader.Characteristics = iter->Characteristics;
		writer.EmitBlob(&sectionheader, sizeof(sectionheader));
	}

	std::wcout << L"OK\n";
}


//
// Determine if this manager is in charge of a PE section
//
bool PESectionManager::RepresentsPESection() const
{
	return false;
}


//
// Determine how many bytes it will take to store the section table
//
size_t PESectionManager::GetSectionSize() const
{
	return (sizeof(IMAGE_SECTION_HEADER) * SectionInfoTable.size());
}


//
// Add a PE file section
//
void PESectionManager::AddSection(const PESectionInfo& sectioninfo, const Linker& linker)
{
	SectionInfoTable.push_back(sectioninfo);
	SectionInfoTable.back().VirtualLocation = VirtualLocationOffset;
	VirtualLocationOffset += linker.RoundUpToVirtualPadding(SectionInfoTable.back().VirtualSize);
}

//
// Retrieve a PE file section by name
//
const PESectionInfo& PESectionManager::GetSection(const char* sectionname) const
{
	std::string sectionnamestr(sectionname);
	for(std::list<PESectionInfo>::const_iterator iter = SectionInfoTable.begin(); iter != SectionInfoTable.end(); ++iter)
	{
		if(iter->SectionName == sectionnamestr)
			return *iter;
	}

	throw Exception("PE file section has not been defined!");
}

//
// Compute how much space we are taking up, when realigned to the
// virtual alignment. Since the virtual alignment exceeds the
// file's physical alignment, this will probably be larger than
// the physical file size.
//
DWORD PESectionManager::GetVirtualImageSize(const Linker& linker) const
{
	DWORD size = linker.RoundUpToVirtualPadding(SectionInfoTable.front().Location);
	for(std::list<PESectionInfo>::const_iterator iter = SectionInfoTable.begin(); iter != SectionInfoTable.end(); ++iter)
		size += linker.RoundUpToVirtualPadding(iter->Size);

	return size;
}
