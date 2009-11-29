//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper objects for overseeing the entire link process
//

#pragma once


// Forward declarations
class Linker;
class PESectionManager;
class ThunkManager;
class PEData;
class Resources;
class CodeGenerator;
class PEHeaderSection;
class LinkWriter;

namespace Projects { class Project; }


//
// This class provides the interface definition that all link section
// managers must implement. It is mainly used for creating an overall
// view of the output file, such as where major data sections will be
// written, offsets to resources, and so on.
//
class LinkerSectionManager
{
// Destruction
public:
	virtual ~LinkerSectionManager()
	{ }

// Section manager interface
public:
	virtual void Generate(Linker& linker) = 0;
	virtual void Emit(Linker& linker, LinkWriter& writer) = 0;

	virtual bool RepresentsPESection() const = 0;
};


//
// This is the general link process manager. It collects data from
// each section manager and uses it to produce a final executable.
//
class Linker
{
// Construction and destruction
public:
	Linker(const Projects::Project& project);
	~Linker();

// EXE generation interface
public:
	void GenerateSections();
	void CommitFile();

// Access to the controlling project settings
public:
	const Projects::Project& GetProject() const
	{ return TheProject; }

// Link information interface
public:
	WORD GetNumberOfSections() const;
	DWORD GetCodeSize() const; 
	DWORD GetDataSize() const;
	DWORD GetEntryPoint() const;
	DWORD GetBaseAddress() const;
	DWORD GetEpochCodeSize() const;

	PESectionManager& GetSectionManager()
	{ return *SectionManager; }

	ThunkManager& GetThunkManager()
	{ return *TheThunkManager; }

	PEData& GetDataManager()
	{ return *TheDataManager; }

	Resources& GetResourceManager()
	{ return *TheResourceManager; }

	PEHeaderSection& GetHeaderManager()
	{ return *HeaderManager; }

// Additional helpers
public:
	DWORD RoundUpToFilePadding(DWORD in) const;
	DWORD RoundUpToVirtualPadding(DWORD in) const;
	DWORD GetFileAlignment() const;
	DWORD GetVirtualAlignment() const;

	DWORD GetVirtualImageSize() const;

// Internal tracking
private:
	const Projects::Project& TheProject;

	std::list<LinkerSectionManager*> SectionManagers;
	
	DWORD CodeSize;
	DWORD DataSize;
	DWORD EntryPointAddress;

	PEHeaderSection* HeaderManager;
	PESectionManager* SectionManager;
	ThunkManager* TheThunkManager;
	CodeGenerator* TheCodeManager;
	PEData* TheDataManager;
	Resources* TheResourceManager;

	static const DWORD FileAlignment;
	static const DWORD VirtualAlignment;
	static const DWORD ImageBaseAddress;
};

