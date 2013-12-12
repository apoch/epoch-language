//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for emitting the Portable Executable header set
//

#include "pch.h"

#include "PEHeader.h"
#include "PESections.h"
#include "Thunk.h"
#include "Resources.h"
#include "Linker/LinkWriter.h"

#include <iostream>


//
// Generate the headers section. We don't really have anything to do.
//
void PEHeaderSection::Generate(Linker&)
{
	std::wcout << L"Generating PE headers... OK\n";
}


//
// Emit the actual PE header data to the output file
//
void PEHeaderSection::Emit(Linker& linker, LinkWriter& writer) const
{
	std::wcout << L"Writing PE headers... ";

	IMAGE_DOS_HEADER DosHeader;
	::ZeroMemory(&DosHeader, sizeof(DosHeader));
	DosHeader.e_magic = 0x5A4D;
	DosHeader.e_cblp = 0x90;
	DosHeader.e_cp = 0x03;
	DosHeader.e_cparhdr = 0x04;
	DosHeader.e_minalloc = 0x00;
	DosHeader.e_maxalloc = 0xffff;
	DosHeader.e_sp = 0xb8;
	DosHeader.e_lfarlc = 0x40;
	DosHeader.e_lfanew = 0xb0;
	writer.EmitBlob(&DosHeader, sizeof(DosHeader));

	unsigned char stub[] = 
	{
		0x0e, 0x1f, 0xBA, 0x0e, 0x00, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x01, 0x4c, 0xcd, 0x21,		// Message output instructions

		0x54, 0x68, 0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72, 0x61, 0x6d,	0x20, 0x69,		// Message data
		0x73, 0x20, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x74,
		0x75, 0x72, 0x65, 0x2e, 0x0d, 0x0a, 0x49, 0x74, 0x20, 0x77, 0x69, 0x6c, 0x6c, 0x20,
		0x6e, 0x6f, 0x74, 0x20, 0x72, 0x75, 0x6e, 0x20, 0x6f, 0x6e, 0x20, 0x79, 0x6f, 0x75,
		0x72, 0x20, 0x70, 0x72, 0x69, 0x6d, 0x69, 0x74, 0x69, 0x76, 0x65, 0x20, 0x63, 0x6f,
		0x6d, 0x70, 0x75, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65,
		0x2e, 0x0d, 0x0a, 0x24
	};

	writer.EmitBlob(stub, sizeof(stub));
	writer.Pad(0xb0);

	IMAGE_NT_HEADERS32 NTHeaders;

	NTHeaders.Signature = 0x00004550;
	::ZeroMemory(&NTHeaders.FileHeader, sizeof(NTHeaders.FileHeader));
	NTHeaders.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
	NTHeaders.FileHeader.NumberOfSections = linker.GetNumberOfSections();
	NTHeaders.FileHeader.PointerToSymbolTable = 0;
	NTHeaders.FileHeader.NumberOfSymbols = 0;
	NTHeaders.FileHeader.TimeDateStamp = 0x00000000;
	NTHeaders.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
	NTHeaders.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_RELOCS_STRIPPED;

	::ZeroMemory(&NTHeaders.OptionalHeader, sizeof(NTHeaders.OptionalHeader));
	NTHeaders.OptionalHeader.Magic = 0x010b;
	NTHeaders.OptionalHeader.MajorLinkerVersion = 0x01;
	NTHeaders.OptionalHeader.MinorLinkerVersion = 0x01;
	NTHeaders.OptionalHeader.SizeOfCode = linker.RoundUpToVirtualPadding(linker.GetCodeSize());
	NTHeaders.OptionalHeader.SizeOfInitializedData = linker.RoundUpToVirtualPadding(linker.GetDataSize());
	NTHeaders.OptionalHeader.SizeOfUninitializedData = 0;
	NTHeaders.OptionalHeader.AddressOfEntryPoint = linker.GetEntryPoint();
	NTHeaders.OptionalHeader.BaseOfCode = linker.GetSectionManager().GetSection(".text").Location;
	NTHeaders.OptionalHeader.BaseOfData = linker.RoundUpToVirtualPadding(linker.GetSectionManager().GetSection(".data").Location);
	NTHeaders.OptionalHeader.ImageBase = linker.GetBaseAddress();
	NTHeaders.OptionalHeader.SectionAlignment = linker.GetVirtualAlignment();
	NTHeaders.OptionalHeader.FileAlignment = linker.GetFileAlignment();
	NTHeaders.OptionalHeader.MajorOperatingSystemVersion = 0x04;
	NTHeaders.OptionalHeader.MinorOperatingSystemVersion = 0x00;
	NTHeaders.OptionalHeader.MajorImageVersion = 0;
	NTHeaders.OptionalHeader.MinorImageVersion = 0;
	NTHeaders.OptionalHeader.MajorSubsystemVersion = 0x04;
	NTHeaders.OptionalHeader.MinorSubsystemVersion = 0;
	NTHeaders.OptionalHeader.Win32VersionValue = 0;
	NTHeaders.OptionalHeader.SizeOfImage = linker.GetVirtualImageSize();
	NTHeaders.OptionalHeader.SizeOfHeaders = GetHeaderSize();
	NTHeaders.OptionalHeader.CheckSum = 0xf00d; // it looks like Windows doesn't even validate this, so we just write whatever we feel like
	NTHeaders.OptionalHeader.Subsystem = (ConsoleMode ? IMAGE_SUBSYSTEM_WINDOWS_CUI : IMAGE_SUBSYSTEM_WINDOWS_GUI);
	NTHeaders.OptionalHeader.DllCharacteristics = 0;
	NTHeaders.OptionalHeader.SizeOfStackReserve = 0x800000;
	NTHeaders.OptionalHeader.SizeOfStackCommit = 0x80000;
	NTHeaders.OptionalHeader.SizeOfHeapReserve = 0x500000;
	NTHeaders.OptionalHeader.SizeOfHeapCommit = 0x50000;
	NTHeaders.OptionalHeader.LoaderFlags = 0;
	NTHeaders.OptionalHeader.NumberOfRvaAndSizes = 0x10;

	NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = linker.GetSectionManager().GetSection(".idata").VirtualLocation + linker.GetThunkManager().GetThunkTableOffset();
	NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = linker.GetThunkManager().GetThunkTableSize();

	NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = linker.GetSectionManager().GetSection(".rsrc").VirtualLocation;
	NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = linker.GetResourceManager().GetSize();

	writer.EmitBlob(&NTHeaders, sizeof(NTHeaders));

	std::wcout << L"OK\n";
}

//
// Determine if this manager is in charge of a PE section
//
bool PEHeaderSection::RepresentsPESection() const
{
	return false;
}

//
// Determine how many bytes we reserve for the DOS and PE headers
//
DWORD PEHeaderSection::GetHeaderSize() const
{
	return 0x400;
}
