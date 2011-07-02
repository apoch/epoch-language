//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for embedding Epoch bytecode in the executable
//

#include "pch.h"

#include "Section Managers/EpochData.h"
#include "Section Managers/PESections.h"

#include "Linker/LinkWriter.h"

#include <iostream>
#include <fstream>


//
// Construct the bytecode writer
//
EpochCode::EpochCode(const std::wstring& filename)
	: Filename(filename)
{
}

//
// Generate any information needed to fill in the file section
//
void EpochCode::Generate(Linker& linker)
{
	std::wcout << L"Generating Epoch bytecode... ";
	
	PESectionInfo sectioninfo;
	sectioninfo.SectionName = ".epoch";
	sectioninfo.Size = linker.RoundUpToFilePadding(linker.GetEpochCodeSize());
	sectioninfo.VirtualSize = linker.GetEpochCodeSize();
	sectioninfo.Location = linker.RoundUpToFilePadding(linker.GetSectionManager().GetEndOfLastSection());
	sectioninfo.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ; 
	linker.GetSectionManager().AddSection(sectioninfo, linker);

	std::wcout << L"OK\n";
}

//
// Write code into the executable
//
void EpochCode::Emit(Linker& linker, LinkWriter& writer) const
{
	std::wcout << L"Writing Epoch code block... ";

	writer.Pad(linker.GetSectionManager().GetSection(".epoch").Location);

	std::ifstream infile(Filename.c_str(), std::ios::binary);
	infile.unsetf(std::ios::skipws);

	if(!infile)
		throw FatalException("Failed to load intermediate binary file");

	unsigned char byte;
	while(infile >> byte)
		writer.EmitByte(byte);

	writer.Pad(linker.RoundUpToFilePadding(linker.GetSectionManager().GetSection(".epoch").Location + linker.GetEpochCodeSize()));

	std::wcout << L"OK\n";
}


//
// Determine if this manager is in charge of a PE section
//
bool EpochCode::RepresentsPESection() const
{
	return true;
}

