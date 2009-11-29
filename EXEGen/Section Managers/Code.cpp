//
// The Epoch Language Project
// Win32 EXE Generator
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
void CodeGenerator::Emit(Linker& linker, LinkWriter& writer)
{
	std::wcout << L"Writing Epoch launcher code... ";
	const ThunkManager& thunk = linker.GetThunkManager();
	const PEData& data = linker.GetDataManager();

	writer.Pad(linker.GetSectionManager().GetSection(".text").Location);

	writer.EmitByte(0x81);			// SUB ESP, 404
	writer.EmitByte(0xec);
	writer.EmitDWORD(0x404);

	writer.EmitByte(0x68);			// PUSH "fuguedll.dll"
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FUGUEDLLNAME, linker));

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<LoadLibraryW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("LoadLibraryW") + linker.GetBaseAddress());

	writer.EmitByte(0x85);			// TEST EAX,EAX
	writer.EmitByte(0xc0);

	writer.EmitByte(0x75);			// JNZ SHORT <skips failure to load message>
	writer.EmitByte(0x18);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to load Fugue...>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILEDFUGUEDLL, linker));

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0xe9);			// JMP <exit>
	writer.EmitDWORD(0xea);

	writer.EmitByte(0x55);			// PUSH EBP

	writer.EmitByte(0x68);			// PUSH <ExecuteBinaryBuffer>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfNarrowString(STRINGS_EXECUTEBINBUFFER, linker));

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<GetProcAddress>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("GetProcAddress") + linker.GetBaseAddress());

	writer.EmitByte(0x8b);			// MOV EBP, EAX
	writer.EmitByte(0xe8);

	writer.EmitByte(0x85);			// TEST EBP, EBP
	writer.EmitByte(0xed);

	writer.EmitByte(0x75);			// JNZ SHORT <skips failure to load message>
	writer.EmitByte(0x18);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <One or more Epoch service functions...>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILEDFUNCTIONS, linker));

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0xe9);			// JMP <exit>
	writer.EmitDWORD(0xc1);

	writer.EmitByte(0x53);			// PUSH EBX
	writer.EmitByte(0x68);			// PUSH 1ff
	writer.EmitDWORD(0x1ff);

	writer.EmitByte(0x8d);			// LEA EAX, DWORD PTR SS:[ESP+C]
	writer.EmitByte(0x44);
	writer.EmitByte(0x24);
	writer.EmitByte(0x0c);

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<GetModuleFileNameW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("GetModuleFileNameW") + linker.GetBaseAddress());

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH 20
	writer.EmitByte(0x20);

	writer.EmitByte(0x6a);			// PUSH 3
	writer.EmitByte(0x03);

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH 1
	writer.EmitByte(0x01);

	writer.EmitByte(0x68);			// PUSH 80000000
	writer.EmitDWORD(0x80000000);

	writer.EmitByte(0x8d);			// LEA ECX, DWORD PTR SS:[ESP+20]
	writer.EmitByte(0x4c);
	writer.EmitByte(0x24);
	writer.EmitByte(0x20);

	writer.EmitByte(0x51);			// PUSH ECX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<CreateFileW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("CreateFileW") + linker.GetBaseAddress());

	writer.EmitByte(0x8b);			// MOV EBX, EAX
	writer.EmitByte(0xd8);

	writer.EmitByte(0x83);			// CMP EBX, -1
	writer.EmitByte(0xfb);
	writer.EmitByte(0xff);

	writer.EmitByte(0x75);			// JNZ SHORT <skip error msg>
	writer.EmitByte(0x16);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to open .EXE>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILEDEXE, linker));

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0xeb);			// JMP SHORT <exit>
	writer.EmitByte(0x76);

	writer.EmitByte(0x57);			// PUSH EDI

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH 2
	writer.EmitByte(0x02);

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x53);			// PUSH EBX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<CreateFileMappingW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("CreateFileMappingW") + linker.GetBaseAddress());

	writer.EmitByte(0x8b);			// MOV EDI, EAX
	writer.EmitByte(0xf8);

	writer.EmitByte(0x85);			// TEST EDI, EDI
	writer.EmitByte(0xff);

	writer.EmitByte(0x75);			// JNZ SHORT <skip error msg>
	writer.EmitByte(0x15);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to map file>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILMAP, linker));

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0xeb);			// JMP SHORT <exit>
	writer.EmitByte(0x48);

	writer.EmitByte(0x56);			// PUSH ESI


	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH	0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH 0
	writer.EmitByte(0x00);

	writer.EmitByte(0x6a);			// PUSH 4
	writer.EmitByte(0x04);

	writer.EmitByte(0x57);			// PUSH EDI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MapViewOfFile>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MapViewOfFile") + linker.GetBaseAddress());

	writer.EmitByte(0x8b);			// MOV ESI, EAX
	writer.EmitByte(0xf0);

	writer.EmitByte(0x85);			// TEST ESI, ESI
	writer.EmitByte(0xf6);

	writer.EmitByte(0x75);			// JNZ SHORT <skip error>
	writer.EmitByte(0x15);

	writer.EmitByte(0x6A);			// PUSH 10
	writer.EmitByte(0x10);

	writer.EmitByte(0x68);			// PUSH <Epoch Subsystem>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_EPOCHSUBSYSTEM, linker));

	writer.EmitByte(0x68);			// PUSH <Failed to map view>
	writer.EmitDWORD(linker.GetBaseAddress() + data.GetOffsetOfWideString(STRINGS_FAILVIEW, linker));

	writer.EmitByte(0x50);			// PUSH EAX

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<MessageBoxW>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("MessageBoxW") + linker.GetBaseAddress());

	writer.EmitByte(0xeb);			// JMP SHORT <exit>
	writer.EmitByte(0x1c);

	writer.EmitByte(0x8d);			// LEA EDX, DWORD PTR DS:[ESI+<code location offset>]
	writer.EmitByte(0x96);
	writer.EmitDWORD(linker.GetSectionManager().GetSection(".epoch").Location);

	writer.EmitByte(0x52);			// PUSH EDX

	writer.EmitByte(0xff);			// CALL EBP <ExecuteBinaryBuffer>
	writer.EmitByte(0xd5);

	writer.EmitByte(0x56);			// PUSH ESI

	writer.EmitByte(0xff);			// CALL DWORD PTR DS:[<UnmapViewOfFile>]
	writer.EmitByte(0x15);
	writer.EmitDWORD(thunk.GetThunkAddress("UnmapViewOfFile") + linker.GetBaseAddress());

	writer.EmitByte(0x8b);			// MOV ESI, DWORD PTR DS:[<CloseHandle>]
	writer.EmitByte(0x35);
	writer.EmitDWORD(thunk.GetThunkAddress("CloseHandle") + linker.GetBaseAddress());

	writer.EmitByte(0x57);			// PUSH EDI

	writer.EmitByte(0xff);			// CALL ESI
	writer.EmitByte(0xd6);

	writer.EmitByte(0x53);			// PUSH EBX

	writer.EmitByte(0xff);			// CALL ESI
	writer.EmitByte(0xd6);

	writer.EmitByte(0x5e);			// POP ESI
	writer.EmitByte(0x5f);			// POP EDI
	writer.EmitByte(0x5b);			// POP EBX
	writer.EmitByte(0x5d);			// POP EBP
	
	// The following set of NOPs replaces a bit of epilogue code
	// generated by the compiler when the launcher stub was first
	// built. We don't need the epilogue code but we keep the ops
	// size the same so that we don't have to recompute jump targets.
	// Yes, I really am that lazy.

	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP
	writer.EmitByte(0x90);			// NOP

	writer.EmitByte(0x81);			// ADD ESP, 404
	writer.EmitByte(0xc4);
	writer.EmitDWORD(0x404);

	writer.EmitByte(0x6A);			// PUSH 0
	writer.EmitByte(0x00);

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
