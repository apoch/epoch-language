//
// The Epoch Language Project
// Win32 EXE Generator
//
// Accelerator resource compiler
//

#include "pch.h"

#include "Resource Compiler/Resource Types/Accelerators.h"

#include "Linker/LinkWriter.h"

#include "Utility/Strings.h"


using namespace ResourceCompiler;


//
// Construct and initialize an accelerator resource wrapper
//
AcceleratorEmitter::AcceleratorEmitter(std::wistream& in)
{
	Table.LoadFromStream(in);
}


//
// Write an accelerator resource to disk
//
void AcceleratorEmitter::Emit(LinkWriter& writer) const
{
	Table.Emit(writer);
}

//
// Determine how much space the accelerator resource requires
//
DWORD AcceleratorEmitter::GetSize() const
{
	return Table.GetTotalSize();
}


void AcceleratorTable::LoadFromStream(std::wistream& in)
{
	std::wstring token;
	in >> token;

	if(token != L"{")
		throw Exception("Incorrect or corrupted menu resource");

	while(in >> token)
	{
		if(in.eof() || token == L"}")
		{
			if(!Entries.empty())
				Entries.back().flags |= 0x80;

			return;
		}

		if(token == L"CTRL")
		{
			Entry entry;
			entry.flags = 0x01 | 0x08;
			
			wchar_t ch;
			in.ignore();
			in >> ch;
			entry.ascii = ch;

			in >> entry.id;
			entry.padding = 0;

			Entries.push_back(entry);
		}
		else if(token == L"VK" || token == L"SHIFT")
		{
			Entry entry;
			entry.flags = 0x01;

			if(token == L"SHIFT")
				entry.flags |= 0x4;

			std::wstring vkeyname;
			in >> vkeyname;
			if(vkeyname == L"F1")
				entry.ascii = VK_F1;
			else if(vkeyname == L"F2")
				entry.ascii = VK_F2;
			else if(vkeyname == L"F3")
				entry.ascii = VK_F3;
			else
				throw FatalException("Unrecognized Virtual Key in accelerator resource");

			in >> entry.id;
			entry.padding = 0;

			Entries.push_back(entry);
		}
		else
			throw Exception("Unrecognized or unexpected token in accelerator resource");
	}
}

DWORD AcceleratorTable::GetTotalSize() const
{
	return Entries.size() * sizeof(Entry);
}

void AcceleratorTable::Emit(LinkWriter& writer) const
{
	for(std::vector<Entry>::const_iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
	{
		writer.EmitWORD(iter->flags);
		writer.EmitWORD(iter->ascii);
		writer.EmitWORD(iter->id);
		writer.EmitWORD(iter->padding);
	}
}

