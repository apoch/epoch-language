//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for writing PE resource blocks
//

#include "pch.h"

#include "Section Managers/Resources.h"
#include "Section Managers/PESections.h"

#include "Linker/LinkWriter.h"

#include "Resource Compiler/ResourceDirectory.h"
#include "Resource Compiler/ResourceScript.h"

#include "Project Files/Project.h"

#include <iostream>


using namespace ResourceCompiler;


//
// Construct and initialize the resource section manager
//
Resources::Resources(Linker& linker)
	: Script(linker.TheProject.GetResourceFileList())
{
}

//
// Generate any information needed to fill in the file section
//
void Resources::Generate(Linker& linker)
{
	std::wcout << L"Generating resource table... ";

	// Load resource directory from resource script
	Script.AddResourcesToDirectory(Directory);
	Directory.ComputeOffsets();

	// Set up the PE resource segment
	PESectionInfo sectioninfo;
	sectioninfo.SectionName = ".rsrc";
	sectioninfo.Size = Directory.GetSize();
	sectioninfo.VirtualSize = sectioninfo.Size;
	sectioninfo.Location = linker.RoundUpToFilePadding(linker.GetSectionManager().GetEndOfLastSection());
	sectioninfo.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ; 
	linker.GetSectionManager().AddSection(sectioninfo, linker);
	
	std::wcout << L"OK\n";
}

//
// Write resources into the executable
//
void Resources::Emit(Linker& linker, LinkWriter& writer)
{
	std::wcout << L"Writing resources... ";

	DWORD start = linker.GetSectionManager().GetSection(".rsrc").Location;
	DWORD virtualstart = linker.GetSectionManager().GetSection(".rsrc").VirtualLocation;
	writer.Pad(start);

	Directory.Emit(writer, virtualstart);

	std::wcout << L"OK\n";
}


//
// Determine if this manager is in charge of a PE section
//
bool Resources::RepresentsPESection() const
{
	return true;
}

//
// Get the total size of embedded resources
//
DWORD Resources::GetSize() const
{
	return Directory.GetSize();
}


