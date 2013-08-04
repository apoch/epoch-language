//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper object for generating the Epoch launcher stub
//

#include "pch.h"

#include "Section Managers/Code.h"
#include "Section Managers/Thunk.h"
#include "Section Managers/PESections.h"
#include "Section Managers/Data.h"

#include "Embedded Resources/EmbeddedStrings.h"

#include "Linker/LinkWriter.h"

#include <iostream>


//
// Generate any information needed to fill in the file section
//
void CodeGenerator::Generate(Linker& linker)
{
	std::wcout << L"Generating Epoch launcher code... ";
	
	PESectionInfo sectioninfo;
	sectioninfo.SectionName = ".text";
	sectioninfo.Size = linker.RoundUpToFilePadding(GetSectionSize());
	sectioninfo.VirtualSize = GetSectionSize();
	sectioninfo.Location = linker.RoundUpToFilePadding(linker.GetSectionManager().GetEndOfLastSection());
	sectioninfo.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ; 
	linker.GetSectionManager().AddSection(sectioninfo, linker);
	
	std::wcout << L"OK\n";
}

//
// Write binary machine code into the executable
//
void CodeGenerator::Emit(Linker& linker, LinkWriter& writer) const
{
	std::wcout << L"Writing Epoch launcher code... ";
	const ThunkManager& thunk = linker.GetThunkManager();
	const PEData& data = linker.GetDataManager();

	writer.Pad(linker.GetSectionManager().GetSection(".text").Location);

	writer.EmitByte(0x55);			// PUSH EBP

	writer.EmitByte(0x8b);			// MOV EBP, ESP
	writer.EmitByte(0xec);

	writer.EmitByte(0x81);			// SUB ESP, 0x404
	writer.EmitByte(0xec);
	writer.EmitDWORD(0x404);

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x68);			// PUSH "epochruntime.dll"
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_RUNTIMEDLLNAME, linker));

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<LoadLibraryW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("LoadLibraryW") + linker.GetBaseAddress());

	writer.EmitByte(0x33);			// XOR ESI, ESI
	writer.EmitByte(0xf6);

	writer.EmitByte(0x3b);			// CMP EAX, ESI
	writer.EmitByte(0xc6);

	writer.EmitByte(0x75);			// JNE <skip error message>
	writer.EmitByte(0x1a);

	writer.EmitByte(0x6a);			// PUSH 0x10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to load runtime...>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILEDRUNTIMEDLL, linker));

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0x33);			// XOR EAX, EAX
	writer.EmitByte(0xc0);

	writer.EmitByte(0xe9);			// JMP <end of function>
	writer.EmitByte(0x8b);
	writer.EmitByte(0x00);
	writer.EmitByte(0x00);
	writer.EmitByte(0x00);

	writer.EmitByte(0x68);			// PUSH <ExecuteBinaryBuffer>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfNarrowString(STRINGS_EXECUTEBINBUFFER, linker));

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<GetProcAddress>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("GetProcAddress") + linker.GetBaseAddress());

	writer.EmitByte(0x89);			// MOV <procaddr>, EAX
	writer.EmitByte(0x45);
	writer.EmitByte(0xfc);
	
	writer.EmitByte(0x3b);			// CMP EAX, ESI
	writer.EmitByte(0xc6);

	writer.EmitByte(0x75);			// JNE <skip error>
	writer.EmitByte(0x0e);

	writer.EmitByte(0x6A);			// PUSH 0x10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <One or more Epoch service functions...>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILEDFUNCTIONS, linker));

	writer.EmitByte(0xeb);			// JMP <show error message>
	writer.EmitByte(0xd1);

	writer.EmitByte(0x53);			// PUSH EBX
	
	writer.EmitByte(0x68);			// PUSH 0x1ff
	writer.EmitDWORD(0x1ff);

	writer.EmitByte(0x8d);			// LEA EAX, <filebuffer>
	writer.EmitByte(0x85);
	writer.EmitByte(0xfc);
	writer.EmitByte(0xfb);
	writer.EmitByte(0xff);
	writer.EmitByte(0xff);

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<GetModuleFileNameW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("GetModuleFileNameW") + linker.GetBaseAddress());

	writer.EmitByte(0x56);			// PUSH ESI
	
	writer.EmitByte(0x6a);			// PUSH 0x20
	writer.EmitByte(0x20);

	writer.EmitByte(0x6a);			// PUSH 0x3
	writer.EmitByte(0x03);

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x6a);			// PUSH 0x1
	writer.EmitByte(0x01);

	writer.EmitByte(0x68);			// PUSH 0x80000000
	writer.EmitDWORD(0x80000000);

	writer.EmitByte(0x8d);			// LEA EAX, <filebuffer>
	writer.EmitByte(0x85);
	writer.EmitByte(0xfc);
	writer.EmitByte(0xfb);
	writer.EmitByte(0xff);
	writer.EmitByte(0xff);

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<CreateFileW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("CreateFileW") + linker.GetBaseAddress());

	writer.EmitByte(0x8b);			// MOV EBX, EAX
	writer.EmitByte(0xd8);

	writer.EmitByte(0x83);			// CMP EBX, -1
	writer.EmitByte(0xfb);
	writer.EmitByte(0xff);

	writer.EmitByte(0x75);			// JNE <skip error msg>
	writer.EmitByte(0x0e);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to open .EXE>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILEDEXE, linker));

	writer.EmitByte(0xeb);			// JMP <show error>
	writer.EmitByte(0x1d);

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x6a);			// PUSH 0x2
	writer.EmitByte(0x02);

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x53);			// PUSH EBX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<CreateFileMappingW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("CreateFileMappingW") + linker.GetBaseAddress());

	writer.EmitByte(0x3b);			// CMP EAX, ESI
	writer.EmitByte(0xc6);

	writer.EmitByte(0x75);			// JNE <skip error>
	writer.EmitByte(0x19);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to map file>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILMAP, linker));

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0x33);			// XOR EAX, EAX
	writer.EmitByte(0xc0);

	writer.EmitByte(0x5b);			// POP EBX
	
	writer.EmitByte(0x5e);			// POP ESI
	
	writer.EmitByte(0xc9);			// LEAVE
	
	writer.EmitByte(0xc3);			// RET

	writer.EmitByte(0x57);			// PUSH EDI

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0x6a);			// PUSH 0x1
	writer.EmitByte(0x01);

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MapViewOfFile>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MapViewOfFile") + linker.GetBaseAddress());

	writer.EmitByte(0x8b);			// MOV EDI, EAX
	writer.EmitByte(0xf8);

	writer.EmitByte(0x3b);			// CMP EDI, ESI
	writer.EmitByte(0xfe);

	writer.EmitByte(0x75);			// JNE <skip error>
	writer.EmitByte(0x18);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to map view>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILVIEW, linker));

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0x33);			// XOR EAX, EAX
	writer.EmitByte(0xc0);

	writer.EmitByte(0x5f);			// POP EDI

	writer.EmitByte(0xeb);			// JMP <exit>
	writer.EmitByte(0xd1);

	writer.EmitByte(0x68);			// PUSH <size of bytecode>
	writer.EmitDWORD(linker.GetEpochCodeSize());

	writer.EmitByte(0x8d);			// LEA EAX, [EDI+offset of bytecode]
	writer.EmitByte(0x87);
	writer.EmitDWORD(linker.GetSectionManager().GetSection(".epoch").Location);

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL <procaddr>
	writer.EmitByte(0x55);
	writer.EmitByte(0xfc);

	writer.EmitByte(0x57);			// PUSH EDI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<UnmapViewOfFile>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("UnmapViewOfFile") + linker.GetBaseAddress());

	writer.EmitByte(0x53);			// PUSH EBX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<UnmapViewOfFile>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("CloseHandle") + linker.GetBaseAddress());

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<ExitProcess>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("ExitProcess") + linker.GetBaseAddress());

	std::wcout << L"OK\n";
}


//
// Determine if this manager is in charge of a PE section
//
bool CodeGenerator::RepresentsPESection() const
{
	return true;
}

//
// Determine how many bytes it will take to store the code
//
DWORD CodeGenerator::GetSectionSize() const
{
	return 0x150;
}

//
// Determine the number of bytes between the start of the code block
// and the start of the actual entry point function
//
DWORD CodeGenerator::GetEntryPointOffset() const
{
	return 0;
}
