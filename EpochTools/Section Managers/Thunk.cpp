//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper objects for building thunk tables that
// store dynamically linked function addresses
//

#include "pch.h"

#include "Thunk.h"
#include "PESections.h"
#include "PEHeader.h"
#include "Linker/LinkWriter.h"

#include <iostream>


//
// Generate any information needed to fill in the file section
//
void ThunkManager::Generate(Linker& linker)
{
	std::wcout << L"Generating thunk tables... ";

	AddThunkFunction("MessageBoxW", "user32.dll");

	AddThunkFunction("LoadLibraryW", "kernel32.dll");
	AddThunkFunction("GetProcAddress", "kernel32.dll");
	AddThunkFunction("GetModuleFileNameW", "kernel32.dll");
	AddThunkFunction("CreateFileW", "kernel32.dll");
	AddThunkFunction("CreateFileMappingW", "kernel32.dll");
	AddThunkFunction("MapViewOfFile", "kernel32.dll");
	AddThunkFunction("UnmapViewOfFile", "kernel32.dll");
	AddThunkFunction("CloseHandle", "kernel32.dll");
	AddThunkFunction("ExitProcess", "kernel32.dll");

	DWORD baseaddress = linker.RoundUpToVirtualPadding(linker.GetHeaderManager().GetHeaderSize());
	DWORD count = 0;

	for(std::map<std::string, DWORD>::iterator iter = Libraries.begin(); iter != Libraries.end(); ++iter)
	{
		for(std::map<std::string, std::pair<DWORD, std::string> >::iterator funciter = Functions.begin(); funciter != Functions.end(); ++funciter)
		{
			if(funciter->second.second == iter->first)
			{
				funciter->second.first = baseaddress + count;
				count += static_cast<DWORD>(funciter->first.size() + 3);		// 1 NULL terminator and 2 prefix hint bytes
			}
		}

		iter->second = baseaddress + count;
		count += static_cast<DWORD>(iter->first.size() + 1);
	}

	for(std::map<std::string, DWORD>::iterator iter = Libraries.begin(); iter != Libraries.end(); ++iter)
	{
		LibraryThunkSpots[iter->first].first = count;
		for(std::map<std::string, std::pair<DWORD, std::string> >::iterator funciter = Functions.begin(); funciter != Functions.end(); ++funciter)
		{
			if(funciter->second.second == iter->first)
				count += sizeof(DWORD);
		}
		count += sizeof(DWORD);
		LibraryThunkSpots[iter->first].second = count - LibraryThunkSpots[iter->first].first;
	}

	for(std::map<std::string, DWORD>::iterator iter = Libraries.begin(); iter != Libraries.end(); ++iter)
	{
		LibraryRewriteThunkSpots[iter->first].first = count;
		for(std::map<std::string, std::pair<DWORD, std::string> >::iterator funciter = Functions.begin(); funciter != Functions.end(); ++funciter)
		{
			if(funciter->second.second == iter->first)
			{
				FunctionThunkLocations.insert(std::make_pair(MakeMagic(iter->first, funciter->first), count + baseaddress));
				count += sizeof(DWORD);
			}
		}
		count += sizeof(DWORD);
		LibraryRewriteThunkSpots[iter->first].second = count - LibraryRewriteThunkSpots[iter->first].first;
	}

	ThunkTableOffset = linker.RoundUpToFilePadding(count);

	count += static_cast<DWORD>(sizeof(IMAGE_IMPORT_DESCRIPTOR) * Libraries.size());

	DataSize = count + GetThunkTableOffset();

	PESectionInfo sectioninfo;
	sectioninfo.SectionName = ".idata";
	sectioninfo.Size = linker.RoundUpToFilePadding(DataSize);
	sectioninfo.VirtualSize = DataSize;
	sectioninfo.Location = linker.GetHeaderManager().GetHeaderSize();
	sectioninfo.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE; 
	linker.GetSectionManager().AddSection(sectioninfo, linker);

	std::wcout << L"OK\n";
}

//
// Write thunk tables into the executable
//
void ThunkManager::Emit(Linker& linker, LinkWriter& writer)
{
	std::wcout << L"Writing thunk tables... ";

	DWORD baseaddress = linker.GetHeaderManager().GetHeaderSize();
	writer.Pad(baseaddress);

	DWORD virtualaddress = linker.RoundUpToVirtualPadding(baseaddress);

	for(std::map<std::string, DWORD>::iterator iter = Libraries.begin(); iter != Libraries.end(); ++iter)
	{
		for(std::map<std::string, std::pair<DWORD, std::string> >::iterator funciter = Functions.begin(); funciter != Functions.end(); ++funciter)
		{
			if(funciter->second.second == iter->first)
			{
				writer.EmitWORD(0);
				writer.EmitNarrowString(funciter->first);
				virtualaddress += static_cast<DWORD>(sizeof(WORD)) + funciter->first.length() + 1;
			}
		}

		writer.EmitNarrowString(iter->first);
		virtualaddress += static_cast<DWORD>(iter->first.length()) + 1;
	}

	for(std::map<std::string, DWORD>::iterator iter = Libraries.begin(); iter != Libraries.end(); ++iter)
	{
		LibraryThunkSpots[iter->first].first = virtualaddress;
		for(std::map<std::string, std::pair<DWORD, std::string> >::iterator funciter = Functions.begin(); funciter != Functions.end(); ++funciter)
		{
			if(funciter->second.second == iter->first)
			{
				writer.EmitDWORD(funciter->second.first);
				virtualaddress += static_cast<DWORD>(sizeof(DWORD));
			}
		}
		writer.EmitDWORD(0);
		virtualaddress += sizeof(DWORD);
	}

	for(std::map<std::string, DWORD>::iterator iter = Libraries.begin(); iter != Libraries.end(); ++iter)
	{
		LibraryRewriteThunkSpots[iter->first].first = virtualaddress;
		for(std::map<std::string, std::pair<DWORD, std::string> >::iterator funciter = Functions.begin(); funciter != Functions.end(); ++funciter)
		{
			if(funciter->second.second == iter->first)
			{
				writer.EmitDWORD(funciter->second.first);
				virtualaddress += sizeof(DWORD);
			}
		}
		writer.EmitDWORD(0);
		virtualaddress += sizeof(DWORD);
	}

	writer.Pad(baseaddress + GetThunkTableOffset());

	for(std::map<std::string, DWORD>::iterator iter = Libraries.begin(); iter != Libraries.end(); ++iter)
	{
		IMAGE_IMPORT_DESCRIPTOR descriptor;
		::ZeroMemory(&descriptor, sizeof(descriptor));
		descriptor.OriginalFirstThunk = LibraryThunkSpots[iter->first].first;
		descriptor.TimeDateStamp = 0;
		descriptor.ForwarderChain = 0;
		descriptor.Name = Libraries[iter->first];
		descriptor.FirstThunk = LibraryRewriteThunkSpots[iter->first].first;
		writer.EmitBlob(&descriptor, sizeof(descriptor));
	}

	std::wcout << L"OK\n";
}


//
// Determine if this manager is in charge of a PE section
//
bool ThunkManager::RepresentsPESection() const
{
	return true;
}

//
// Add an imported function to the list
//
void ThunkManager::AddThunkFunction(const std::string& functionname, const std::string& libraryname)
{
	if(Libraries.find(libraryname) == Libraries.end())
		Libraries.insert(std::make_pair(libraryname, 0));

	if(Functions.find(functionname) == Functions.end())
		Functions.insert(std::make_pair(functionname, std::make_pair(0, libraryname)));
}

//
// Get the address of the thunk table that corresponds to the given function
//
DWORD ThunkManager::GetThunkAddress(const std::string& functionname) const
{
	std::map<std::string, std::pair<DWORD, std::string> >::const_iterator funciter = Functions.find(functionname);
	if(funciter == Functions.end())
		throw Exception("Failed to locate the requested library in the thunk manager table");

	const std::string& libraryname = funciter->second.second;
	std::string magicid = MakeMagic(libraryname, functionname);

	std::map<std::string, DWORD>::const_iterator iter = FunctionThunkLocations.find(magicid);
	if(iter == FunctionThunkLocations.end())
		throw Exception("Failed to locate the requested function in the thunk manager table");

	return iter->second;
}


//
// Generate a unique identifier string for a given function in a given library
//
// We do this to avoid name clashes when multiple libraries expose functions
// with the same names.
//
std::string ThunkManager::MakeMagic(const std::string& library, const std::string& func) const
{
	return func + "@@module:" + library;
}

//
// Get the offset of the actual thunk table's beginning
//
DWORD ThunkManager::GetThunkTableOffset() const
{
	return ThunkTableOffset;
}

//
// Get the total size of the thunk table
//
DWORD ThunkManager::GetThunkTableSize() const
{
	return static_cast<DWORD>(Libraries.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR));
}
