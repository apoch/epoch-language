//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for overseeing the entire link process
//

#include "pch.h"

#include "Linker/LinkWriter.h"

#include "Section Managers/PEHeader.h"
#include "Section Managers/PESections.h"
#include "Section Managers/Thunk.h"
#include "Section Managers/Code.h"
#include "Section Managers/Data.h"
#include "Section Managers/Resources.h"
#include "Section Managers/EpochData.h"

#include "Embedded Resources/EmbeddedStrings.h"

#include <boost/filesystem/operations.hpp>


//-------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------

const DWORD Linker::FileAlignment = 0x200;
const DWORD Linker::VirtualAlignment = 0x1000;
const DWORD Linker::ImageBaseAddress = 0x400000;


//-------------------------------------------------------------------------------
// Linker class implementation
//-------------------------------------------------------------------------------

//
// Construct and initialize the link operation manager
//
Linker::Linker()
	: CodeSize(0),
	  DataSize(0),

	  EntryPointAddress(0),

	  HeaderManager(NULL),
	  SectionManager(NULL),
	  TheThunkManager(NULL),
	  TheCodeManager(NULL),
	  TheDataManager(NULL),
	  TheResourceManager(NULL)
{
	// Note that the ordering here is important - it controls the order of the final EXE output!
	SectionManagers.push_back(new PEHeaderSection);
	HeaderManager = dynamic_cast<PEHeaderSection*>(SectionManagers.back());

	SectionManagers.push_back(new PESectionManager(RoundUpToVirtualPadding(HeaderManager->GetHeaderSize())));
	SectionManager = dynamic_cast<PESectionManager*>(SectionManagers.back());

	SectionManagers.push_back(new ThunkManager);
	TheThunkManager = dynamic_cast<ThunkManager*>(SectionManagers.back());

	SectionManagers.push_back(new CodeGenerator);
	TheCodeManager = dynamic_cast<CodeGenerator*>(SectionManagers.back());

	SectionManagers.push_back(new PEData);
	TheDataManager = dynamic_cast<PEData*>(SectionManagers.back());

	SectionManagers.push_back(new Resources(*this));
	TheResourceManager = dynamic_cast<Resources*>(SectionManagers.back());

	SectionManagers.push_back(new EpochCode(L"d:\\foo.easm"));			// TODO - use real file name from project
}

//
// Destruct and clean up the linker
//
Linker::~Linker()
{
	for(std::list<LinkerSectionManager*>::iterator iter = SectionManagers.begin(); iter != SectionManagers.end(); ++iter)
		delete *iter;
}

//
// Have each link section manager generate its data
//
void Linker::GenerateSections()
{
#define STRING(StringID, StringContent)		TheDataManager->AddString(StringID, StringContent);
	EMBEDDED_STRING_TABLE
#undef STRING

	for(std::list<LinkerSectionManager*>::iterator iter = SectionManagers.begin(); iter != SectionManagers.end(); ++iter)
		(*iter)->Generate(*this);

	EntryPointAddress = TheCodeManager->GetEntryPointOffset() + SectionManager->GetSection(".text").VirtualLocation;
}

//
// Output the final linked file to disk
//
void Linker::CommitFile()
{
	std::ofstream outstream("D:\\foo.exe", std::ios::binary);			// TODO - use real output filename from project
	if(!outstream)
		throw FileException("Cannot open output file for writing");

	outstream.unsetf(std::ios::skipws);

	if(false)			// TODO - set console mode based on project settings
		HeaderManager->SetConsoleMode();

	LinkWriter writer(outstream);

	for(std::list<LinkerSectionManager*>::iterator iter = SectionManagers.begin(); iter != SectionManagers.end(); ++iter)
		(*iter)->Emit(*this, writer);
}


//
// Count how many PE sections are going to be written
//
WORD Linker::GetNumberOfSections() const
{
	WORD ret = 0;
	for(std::list<LinkerSectionManager*>::const_iterator iter = SectionManagers.begin(); iter != SectionManagers.end(); ++iter)
	{
		if((*iter)->RepresentsPESection())
			++ret;
	}

	return ret;
}

//
// Determine how many bytes of code are going to be written
//
DWORD Linker::GetCodeSize() const
{
	return static_cast<DWORD>(TheCodeManager->GetSectionSize());
}

//
// Determine how many bytes of pre-initialized data are going to be written
//
DWORD Linker::GetDataSize() const
{
	return static_cast<DWORD>(TheDataManager->GetSectionSize());
}

//
// Determine the offset of the code entry point
//
DWORD Linker::GetEntryPoint() const
{
	return EntryPointAddress;
}

//
// Determine the default base address for the image
//
DWORD Linker::GetBaseAddress() const
{
	return ImageBaseAddress;
}

//
// Determine how much space the Epoch bytecode consumes
//
DWORD Linker::GetEpochCodeSize() const
{
	boost::intmax_t size = boost::filesystem::file_size("d:\\foo.easm");		// TODO - use real intermediate file names
	return RoundUpToFilePadding(static_cast<DWORD>(size));
}


//
// Round a given value up to the nearest multiple of the file padding size
//
DWORD Linker::RoundUpToFilePadding(DWORD in) const
{
	if(in % FileAlignment == 0)
		return in;

	return ((in / FileAlignment) + 1) * FileAlignment;
}

//
// Round a given value up to the nearest multiple of the virtual (in-memory) padding size
//
DWORD Linker::RoundUpToVirtualPadding(DWORD in) const
{
	if(in % VirtualAlignment == 0)
		return in;

	return ((in / VirtualAlignment) + 1) * VirtualAlignment;
}

//
// Retrieve the current file alignment value
//
DWORD Linker::GetFileAlignment() const
{
	return FileAlignment;
}

//
// Retrieve the current virtual alignment value
//
DWORD Linker::GetVirtualAlignment() const
{
	return VirtualAlignment;
}

//
// Compute the size of the image once it is loaded into memory.
// Since the virtual image alignment is larger than the physical
// alignment on disk, we need to compensate - therefore the
// reported size from this function will probably exceed the
// generated file's physical size.
//
DWORD Linker::GetVirtualImageSize() const
{
	return SectionManager->GetVirtualImageSize(*this);
}

