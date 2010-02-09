//
// The Epoch Language Project
// Win32 EXE Generator
//
// Facilities for debugging compiled Epoch .EXEs
//

#include "pch.h"

#include "Debugging/EXEDebugger.h"


using namespace Debugger;


//
// Construct a debug session wrapper
//
EXEDebugger::EXEDebugger(const std::wstring& filename)
{
	FileHandle = ::CreateFile(filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(FileHandle == INVALID_HANDLE_VALUE)
		throw Exception("Cannot debug the specified .EXE file");

	Mapping = ::CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	if(!Mapping)
	{
		::CloseHandle(FileHandle);
		throw Exception("Cannot map .EXE file for debugging");
	}

	EntireFileBuffer = ::MapViewOfFile(Mapping, FILE_MAP_READ, 0, 0, 0);

	try
	{
		FindInterestingLocations();
	}
	catch(...)
	{
		Cleanup();
		throw;
	}
}


//
// Destruct and clean up a debug session wrapper
//
EXEDebugger::~EXEDebugger()
{
	Cleanup();
}


//
// Helper for tidying up our resources
//
void EXEDebugger::Cleanup()
{
	::UnmapViewOfFile(EntireFileBuffer);
	::CloseHandle(Mapping);
	::CloseHandle(FileHandle);
}


//
// Locate and store the positions of areas of interest in the .EXE file
//
void EXEDebugger::FindInterestingLocations()
{
	const IMAGE_DOS_HEADER* dosheader = reinterpret_cast<const IMAGE_DOS_HEADER*>(EntireFileBuffer);
	const IMAGE_NT_HEADERS32* ntheader = reinterpret_cast<const IMAGE_NT_HEADERS32*>(reinterpret_cast<const char*>(EntireFileBuffer) + dosheader->e_lfanew);

	const IMAGE_SECTION_HEADER* sectionheaders = reinterpret_cast<const IMAGE_SECTION_HEADER*>(reinterpret_cast<const char*>(ntheader) + sizeof(IMAGE_NT_HEADERS32));
	
	BinaryCodeBuffer = NULL;
	std::string epochsectionname(".epoch");
	for(size_t i = 0; i < ntheader->FileHeader.NumberOfSections; ++i)
	{
		if(epochsectionname == reinterpret_cast<const char*>(&sectionheaders[i].Name[0]))
		{
			BinaryCodeBuffer = reinterpret_cast<const char*>(EntireFileBuffer) + sectionheaders[i].PointerToRawData;
			break;
		}
	}

	if(!BinaryCodeBuffer)
		throw Exception("Cannot debug the specified .EXE file - does not appear to contain an Epoch program");
}

