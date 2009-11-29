//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper objects for working with Portable Executable file sections
//

#pragma once

// Dependencies
#include "Linker/Linker.h"


//
// Helper structure for tracking information about a PE section
//
struct PESectionInfo
{
	const char* SectionName;
	DWORD Size;
	DWORD Location;
	DWORD VirtualSize;
	DWORD VirtualLocation;			// This member should not be initialized; it is computed by the section manager
	DWORD Characteristics;
};


//
// Manager for handling PE sections in a linked executable
//
class PESectionManager : public LinkerSectionManager
{
// Construction
public:
	explicit PESectionManager(DWORD startoffset)
		: VirtualLocationOffset(startoffset)
	{ }

// PE section management
public:
	void AddSection(const PESectionInfo& sectioninfo, const Linker& linker);
	DWORD GetEndOfLastSection() const;

	const PESectionInfo& GetSection(const char* sectionname) const;

// Section manager interface
public:
	virtual void Generate(Linker& linker);
	virtual void Emit(Linker& linker, LinkWriter& writer);

	virtual bool RepresentsPESection() const;
	size_t GetSectionSize() const;
	DWORD GetVirtualImageSize(const Linker& linker) const;

// Internal tracking
private:
	std::list<PESectionInfo> SectionInfoTable;
	DWORD VirtualLocationOffset;
};

