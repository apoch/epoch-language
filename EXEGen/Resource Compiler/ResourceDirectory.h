//
// The Epoch Language Project
// Win32 EXE Generator
//
// Helper interfaces for the resource manager
//

#pragma once


// Forward declarations
class LinkWriter;


namespace ResourceCompiler
{

	// Forward declarations
	class ResourceEmitter;


	//
	// Helper interface for internal resource tracking
	//
	class DirectoryBase
	{
	public:
		~DirectoryBase()
		{ }

	public:
		virtual DWORD GetSize() const = 0;
		virtual void Emit(LinkWriter& writer, bool pointstoleaf, DWORD virtualbaseaddress) = 0;
		virtual bool SetOffset(DWORD offset)
		{ return false; }

		static DWORD GetSizeStatic();
	};


	//
	// Helper objects for tracking resource directory nodes
	//
	class DirectoryHeader : public DirectoryBase
	{
	// Construction
	public:
		DirectoryHeader(DWORD characteristics, DWORD timedatestamp, WORD majorversion, WORD minorversion, WORD numnamedentries, WORD numidentries);

	// Directory header fields
	public:
		DWORD Characteristics;
		DWORD TimeDateStamp;
		WORD MajorVersion;
		WORD MinorVersion;
		WORD NumNamedEntries;
		WORD NumIDEntries;

	// DirectoryBase interface
	public:
		virtual DWORD GetSize() const;
		virtual void Emit(LinkWriter& writer, bool pointstoleaf, DWORD virtualbaseaddress);

		static DWORD GetSizeStatic();
	};

	class DirectoryEntry : public DirectoryBase
	{
	// Construction
	public:
		DirectoryEntry(DWORD name, DWORD offsettodata);

	// Directory entry fields
	public:
		DWORD Name;
		DWORD OffsetToData;

	// DirectoryBase interface
	public:
		virtual DWORD GetSize() const;
		virtual void Emit(LinkWriter& writer, bool pointstoleaf, DWORD virtualbaseaddress);
		virtual bool SetOffset(DWORD offset)
		{ OffsetToData = offset; return true; }

		static DWORD GetSizeStatic();
	};

	class DirectoryLeaf : public DirectoryBase
	{
	// Construction
	public:
		DirectoryLeaf(DWORD offsettodata, DWORD size, DWORD codepage, DWORD boundresid);

	// Directory leaf fields
	public:
		DWORD OffsetToData;
		DWORD Size;
		DWORD CodePage;
		DWORD Reserved;
		DWORD BoundResourceID;

	// Directory leaf interface
	public:
		DWORD GetSize() const;
		void Emit(LinkWriter& writer, bool pointstoleaf, DWORD virtualbaseaddress);

		static DWORD GetSizeStatic();
	};


	//
	// General resource directory manager
	//
	class ResourceDirectory
	{
	// Construction and destruction
	public:
		ResourceDirectory();
		~ResourceDirectory();

	// Resource management interface
	public:
		void AddResource(DWORD type, DWORD id, DWORD language, ResourceEmitter* emitter);
		void ComputeOffsets();

	// Linker interface
	public:
		void Emit(LinkWriter& writer, DWORD virtualbaseaddress);
		DWORD GetSize() const;

	// Internal tracking
	private:
		std::list<DirectoryBase*> RootTier;
		std::list<DirectoryBase*> IDTier;
		std::list<DirectoryBase*> LanguageTier;
		std::list<DirectoryBase*> LeafTier;

		std::map<DWORD, ResourceEmitter*> ResourceEmitters;

		DirectoryHeader* RootTierHeader;

		DWORD DirectorySize;
		DWORD ResourceSize;
	};
}


