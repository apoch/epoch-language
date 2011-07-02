//
// The Epoch Language Project
// Win32 EXE Generator
//
// Helper interfaces for the resource manager
//

#pragma once


// Forward declarations
class LinkWriter;


// Dependencies
#include <list>
#include <map>


namespace ResourceCompiler
{

	// Forward declarations
	class ResourceEmitter;
	class DirectoryLeaf;


	//
	// Helper interface for internal resource tracking
	//
	class DirectoryBase
	{
	public:
		virtual ~DirectoryBase()
		{ }

	public:
		virtual DWORD GetSize() const = 0;
		virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const = 0;
		virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const = 0;

		virtual void SetOffsetsAll(DWORD& offset) = 0;
		virtual void SetOffsetsSelf(DWORD& offset) = 0;
		
		void AccumulateOffsets(DWORD& offset);
		bool PointsToLeaf() const;

		void AccumulateLeaves(std::list<DirectoryLeaf*>& leaves) const;

		void EmitChildren(LinkWriter& writer, DWORD virtualbaseaddress) const;
		void SetOffsetsChildren(DWORD& offset);

		std::list<DirectoryBase*> Children;
	};


	//
	// Helper objects for tracking resource directory nodes
	//
	class DirectoryHeader : public DirectoryBase
	{
	// Construction
	public:
		DirectoryHeader(DWORD characteristics, DWORD timedatestamp, WORD majorversion, WORD minorversion);

	// Directory header fields
	public:
		DWORD Characteristics;
		DWORD TimeDateStamp;
		WORD MajorVersion;
		WORD MinorVersion;

	// DirectoryBase interface
	public:
		virtual DWORD GetSize() const;
		virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const;
		virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const;
		virtual void SetOffsetsAll(DWORD& offset);
		virtual void SetOffsetsSelf(DWORD& offset);
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
		virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const;
		virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const;
		virtual void SetOffsetsAll(DWORD& offset);
		virtual void SetOffsetsSelf(DWORD& offset);
	};

	class DirectoryLeaf : public DirectoryBase
	{
	// Construction
	public:
		DirectoryLeaf(DWORD offsettodata, DWORD size, DWORD codepage);

	// Directory leaf fields
	public:
		DWORD OffsetToData;
		DWORD Size;
		DWORD CodePage;
		DWORD Reserved;

	// Directory leaf interface
	public:
		virtual DWORD GetSize() const;
		virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const;
		virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const;
		virtual void SetOffsetsAll(DWORD& offset);
		virtual void SetOffsetsSelf(DWORD& offset);
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
		void Emit(LinkWriter& writer, DWORD virtualbaseaddress) const;
		DWORD GetSize() const;

	// Internal helpers
	private:
		struct ResourceRecord
		{
			ResourceRecord(DWORD type, DWORD id, DWORD language, ResourceEmitter* emitter)
				: Type(type), ID(id), Language(language), Emitter(emitter)
			{ }

			DWORD Type;
			DWORD ID;
			DWORD Language;
			ResourceEmitter* Emitter;
		};

	// Internal tracking
	private:
		std::list<ResourceRecord> ResourceRecords;

		DirectoryHeader* RootTier;

		DWORD DirectorySize;
		DWORD ResourceSize;
	};
}


