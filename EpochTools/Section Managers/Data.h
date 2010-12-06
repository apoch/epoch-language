//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper object for writing PE data blocks
//

#pragma once


// Dependencies
#include "Linker/Linker.h"
#include <map>


//
// Wrapper class for writing constants data to the PE data section
//
class PEData : public LinkerSectionManager
{
// Section manager interface
public:
	virtual void Generate(Linker& linker);
	virtual void Emit(Linker& linker, LinkWriter& writer);

	virtual bool RepresentsPESection() const;
	size_t GetSectionSize() const;

// String management interface
public:
	void AddString(unsigned id, const std::string& str);
	void AddString(unsigned id, const std::wstring& str);

	DWORD GetOffsetOfNarrowString(unsigned id, Linker& linker) const;
	DWORD GetOffsetOfWideString(unsigned id, Linker& linker) const;

// Internal tracking
private:
	std::map<unsigned, std::string> NarrowStrings;
	std::map<unsigned, std::wstring> WideStrings;

	std::map<unsigned, DWORD> NarrowStringOffsets;
	std::map<unsigned, DWORD> WideStringOffsets;

	DWORD SectionSize;
};

