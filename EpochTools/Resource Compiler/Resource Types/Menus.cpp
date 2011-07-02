//
// The Epoch Language Project
// Win32 EXE Generator
//
// Menu resource compiler
//

#include "pch.h"

#include "Resource Compiler/Resource Types/Menus.h"

#include "Linker/LinkWriter.h"

#include "Utility/Strings.h"


using namespace ResourceCompiler;


//
// Prototypes
//
std::wstring StripQuotes(const std::wstring& str);


//
// Construct and initialize a menu resource wrapper
//
MenuEmitter::MenuEmitter(std::wistream& in)
{
	Root.LoadFromStream(in, Root.SubEntries);
}


//
// Write a menu resource to disk
//
void MenuEmitter::Emit(LinkWriter& writer) const
{
	writer.EmitWORD(0x00);		// Version, reserved
	writer.EmitWORD(0x00);		// Header size, reserved

	for(std::list<MenuEntry>::const_iterator iter = Root.SubEntries.begin(); iter != Root.SubEntries.end(); ++iter)
		iter->Emit(writer);
}

//
// Determine how much space the menu resource requires
//
DWORD MenuEmitter::GetSize() const
{
	return Root.GetTotalSize();
}


//
// Obtain the size of a menu entry, including all of its children
// Note that this is the size the resource will consume on disk
// once compiled - NOT the in-memory size.
//
DWORD MenuEntry::GetTotalSize() const
{
	DWORD size = static_cast<DWORD>((Text.length() + 1) * sizeof(wchar_t));

	if(IsPopup)
		size += sizeof(WORD);
	else
		size += sizeof(WORD) * 2;

	for(std::list<MenuEntry>::const_iterator iter = SubEntries.begin(); iter != SubEntries.end(); ++iter)
		size += iter->GetTotalSize();
	
	return size;
}

//
// Load a menu resource from the given input stream
//
void MenuEntry::LoadFromStream(std::wistream& in, std::list<MenuEntry>& entries)
{
	std::wstring token;
	in >> token;

	if(token != L"{")
		throw Exception("Incorrect or corrupted menu resource");

	while(true)
	{
		in >> token;

		if(in.eof() || token == L"}")
		{
			if(!entries.empty())
				entries.back().IsLast = true;

			return;
		}

		if(token == L"popup")
		{
			in.ignore();

			MenuEntry entry;
			entry.IsPopup = true;
			entry.IsLast = false;
			entry.ID = 0;
			std::getline(in, entry.Text);
			entry.Text = StripQuotes(StripWhitespace(entry.Text));

			entries.push_back(entry);
			LoadFromStream(in, entries.back().SubEntries);
		}
		else if(token == L"item")
		{
			MenuEntry entry;
			entry.IsPopup = false;
			in >> entry.ID;
			if(entry.ID)
			{
				in.ignore();
				std::getline(in, entry.Text);
				entry.Text = StripQuotes(StripWhitespace(entry.Text));
			}
			entry.IsLast = false;

			entries.push_back(entry);
		}
		else if(token == L"separator")
		{
			MenuEntry entry;
			entry.IsPopup = entry.IsLast = false;
			entry.ID = 0;
			entries.push_back(entry);
		}
		else
			throw Exception("Unrecognized or unexpected token in menu resource");
	}
}

//
// Write the menu entry tree into the compiled file
//
void MenuEntry::Emit(LinkWriter& writer) const
{
	WORD flags = 0;
	if(IsPopup)
		flags |= MF_POPUP;
	if(IsLast)
		flags |= MF_END;

	if(ID == 0 && Text.empty())		// This is the magic combination that indicates a separator bar
		flags = 0;

	writer.EmitWORD(flags);
	if(!IsPopup)
		writer.EmitWORD(ID);
	
	writer.EmitWideString(Text);

	for(std::list<MenuEntry>::const_iterator iter = SubEntries.begin(); iter != SubEntries.end(); ++iter)
		iter->Emit(writer);
}

