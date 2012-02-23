//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper object for writing PE data blocks
//

#include "pch.h"

#include "Section Managers/Data.h"
#include "Section Managers/PESections.h"

#include "Linker/LinkWriter.h"

#include <iostream>


//
// Generate any information needed to fill in the file section
//
void PEData::Generate(Linker& linker)
{
	std::wcout << L"Generating string lookup tables... ";

	// Generate offset info for all narrow strings first
	NarrowStringOffsets.clear();
	DWORD offset = 0;
	for(std::map<unsigned, std::string>::const_iterator iter = NarrowStrings.begin(); iter != NarrowStrings.end(); ++iter)
	{
		NarrowStringOffsets[iter->first] = offset;
		offset += static_cast<DWORD>(iter->second.length() + 1);
	}

	// Next generate offsets for wide strings
	for(std::map<unsigned, std::wstring>::const_iterator iter = WideStrings.begin(); iter != WideStrings.end(); ++iter)
	{
		WideStringOffsets[iter->first] = offset;
		offset += static_cast<DWORD>((iter->second.length() + 1) * sizeof(std::wstring::value_type));
	}

	SectionSize = offset;
	
	PESectionInfo sectioninfo;
	sectioninfo.SectionName = ".data";
	sectioninfo.Size = linker.RoundUpToFilePadding(static_cast<DWORD>(offset + sizeof(wchar_t)));
	sectioninfo.VirtualSize = static_cast<DWORD>(offset + sizeof(wchar_t));
	sectioninfo.Location = linker.RoundUpToFilePadding(linker.GetSectionManager().GetEndOfLastSection());
	sectioninfo.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ; 
	linker.GetSectionManager().AddSection(sectioninfo, linker);

	std::wcout << L"OK\n";
}

//
// Write string tables into the executable
//
void PEData::Emit(Linker& linker, LinkWriter& writer) const
{
	std::wcout << L"Writing string lookup tables... ";

	writer.Pad(linker.GetSectionManager().GetSection(".data").Location);
	
	for(std::map<unsigned, std::string>::const_iterator iter = NarrowStrings.begin(); iter != NarrowStrings.end(); ++iter)
		writer.EmitNarrowString(iter->second);

	for(std::map<unsigned, std::wstring>::const_iterator iter = WideStrings.begin(); iter != WideStrings.end(); ++iter)
		writer.EmitWideString(iter->second);

	std::wcout << L"OK\n";
}


//
// Determine if this manager is in charge of a PE section
//
bool PEData::RepresentsPESection() const
{
	return true;
}

//
// Determine how many bytes it will take to store the string table
//
size_t PEData::GetSectionSize() const
{
	return SectionSize;
}


//
// Add strings to the data table
//
void PEData::AddString(unsigned id, const std::string& str)
{
	NarrowStrings[id] = str;
}

void PEData::AddString(unsigned id, const std::wstring& str)
{
	WideStrings[id] = str;
}

//
// Retrieve the determined offsets of a given string in the table
// WARNING: call Generate() first or this function will give bogus answers!
//
DWORD PEData::GetOffsetOfNarrowString(unsigned id, Linker& linker) const
{
	std::map<unsigned, DWORD>::const_iterator iter = NarrowStringOffsets.find(id);
	if(iter == NarrowStringOffsets.end())
		throw Exception("Requested the offset of a string which is not in the string pool!");

	return iter->second + linker.GetSectionManager().GetSection(".data").VirtualLocation;
}

DWORD PEData::GetOffsetOfWideString(unsigned id, Linker& linker) const
{
	std::map<unsigned, DWORD>::const_iterator iter = WideStringOffsets.find(id);
	if(iter == WideStringOffsets.end())
		throw Exception("Requested the offset of a string which is not in the string pool!");

	return iter->second + linker.GetSectionManager().GetSection(".data").VirtualLocation;
}
