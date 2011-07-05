//
// The Epoch Language Project
// Win32 EXE Generator
//
// Resource Directory management classes
//

#include "pch.h"

#include "Resource Compiler/ResourceDirectory.h"
#include "Resource Compiler/ResourceEmitter.h"

#include "Linker/LinkWriter.h"


using namespace ResourceCompiler;


//
// Internal implementation details
//
namespace ResourceCompiler
{
	namespace
	{

		// Forward declarations
		class DirectoryEntry;
		class DirectoryLeaf;

		//
		// Helper abstract base class for internal resource tracking
		//
		// In Windows Portable Executable files, resources are cataloged using a tree-like
		// structure known as a directory. Each node in the tree is of one of three types;
		// corresponding derived classes for all can be found below. Note that directories
		// are not a strict tree where each node holds pointers to its child nodes; rather
		// certain nodes have pointers to their children and others list their children in
		// an array-like format. Because of this, some functionality for traversal differs
		// between node classes, such as emission of resource data and offset computation.
		//
		// This base class provides an interface for the different mechanisms of traversal
		// as well as a shared implementation of logic common across all of the node types
		// in a directory. Each member function carries additional specific documentation.
		//
		class DirectoryNode
		{
		// Destruction
		public:
			virtual ~DirectoryNode();

		// Linker data emission
		public:
			virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const = 0;
			virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const = 0;

		// Queries
		public:
			virtual DWORD GetSize() const = 0;

		// Offset calculation (for constructing the on-disk data representation)
		public:
			virtual void SetOffsetsAll(DWORD& offset) = 0;
			virtual void SetOffsetsSelf(DWORD& offset) = 0;

		// Leaf-status query
		public:
			virtual bool IsLeaf() const
			{ return false; }

		// Offset propagation (independent of derived class behaviours)
		public:
			void SetOffsetsChildren(DWORD& offset);
			
		// Offset sanity checking
		public:
			void AccumulateOffsets(DWORD& offset);

		// Emission propagation (independent of derived class behaviours)
		public:
			void EmitChildren(LinkWriter& writer, DWORD virtualbaseaddress) const;

		// Additional queries
		public:
			bool PointsToLeaf() const;

		// Internal tracking
		protected:
			std::list<DirectoryNode*> Children;
		};


		//
		// Header node in the directory
		//
		// A directory header precedes a list of directory entries in the data structure.
		// The entry list is stored in flat array format, so this node type specifies the
		// number of child entry nodes attached to this parent node. For the most part we
		// do not use the header; its data fields are generally left blank, and nodes are
		// inserted into the structure only to maintain the expected layout.
		//
		class DirectoryHeader : public DirectoryNode
		{
		// Construction
		public:
			DirectoryHeader(DWORD characteristics, DWORD timedatestamp, WORD majorversion, WORD minorversion);

		// DirectoryNode interface
		public:
			virtual DWORD GetSize() const;
			virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const;
			virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const;
			virtual void SetOffsetsAll(DWORD& offset);
			virtual void SetOffsetsSelf(DWORD& offset);

		// Child node linkage (restricted to certain node types)
		public:
			DirectoryEntry* AddChild(DirectoryEntry* child);

		// Internal tracking
		protected:
			DWORD Characteristics;
			DWORD TimeDateStamp;
			WORD MajorVersion;
			WORD MinorVersion;
		};

		//
		// Entry node in the directory
		//
		// A directory entry is a child of a directory header node, and is physically
		// located in a flat array immediately following its parent header node. Each
		// entry points to a single child in the directory tree, and supplements that
		// pointer with an (optional) ID number. In some contexts this ID number will
		// be interpreted in certain ways, e.g. a resource ID, language ID code, or a
		// resource type, depending on its location in the tree structure.
		//
		// Children of a directory entry will either be header or leaf nodes, without
		// exception. If the child is a leaf node, it will have no siblings. Also, we
		// must flag the node as either a header (high bit of the offset is set) or a
		// leaf (high bit is cleared) when emitting the resource directory to disk.
		//
		class DirectoryEntry : public DirectoryNode
		{
		// Construction
		public:
			DirectoryEntry(DWORD name, DWORD offsettodata);

		// DirectoryNode interface
		public:
			virtual DWORD GetSize() const;
			virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const;
			virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const;
			virtual void SetOffsetsAll(DWORD& offset);
			virtual void SetOffsetsSelf(DWORD& offset);

		// Child node linkage (restricted to certain node types)
		public:
			DirectoryHeader* AddChild(DirectoryHeader* child);
			DirectoryLeaf* AddChild(DirectoryLeaf* child);

		// Internal tracking
		protected:
			DWORD Name;
			DWORD OffsetToData;
		};

		//
		// Leaf node in the directory
		//
		// Leaves are always singular, i.e. have no siblings. They are children of entry
		// nodes, stored via pointer to an offset rather than a flat array. Each leaf in
		// turn points to the resource data offset and the size of the data itself.
		//
		class DirectoryLeaf : public DirectoryNode
		{
		// Construction
		public:
			DirectoryLeaf(DWORD offsettodata, DWORD size, DWORD codepage);

		// DirectoryNode interface
		public:
			virtual DWORD GetSize() const;
			virtual void EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const;
			virtual void EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const;
			virtual void SetOffsetsAll(DWORD& offset);
			virtual void SetOffsetsSelf(DWORD& offset);

			virtual bool IsLeaf() const
			{ return true; }

		// Offset adjustment
		public:
			void SetOffsetToData(DWORD offset);

		// Internal tracking
		protected:
			DWORD OffsetToData;
			DWORD Size;
			DWORD CodePage;
			DWORD Reserved;
		};

	}
}


//
// Destruct and clean up a resource directory node
//
DirectoryNode::~DirectoryNode()
{
	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		delete *iter;
}

//
// Accumulate the total offset from the beginning of a directory node to the end of all of its descendants
//
// This is primarily a sanity check to ensure that the offset computed by the tree traversal is valid. The
// offset calculated here should be the offset from the beginning of the directory tree structure on disk,
// to the beginning of resource data on disk.
//
void DirectoryNode::AccumulateOffsets(DWORD& offset)
{
	offset += GetSize();
	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->AccumulateOffsets(offset);
}

//
// Helper query: determine if a node has exactly one DirectoryLeaf child
//
bool DirectoryNode::PointsToLeaf() const
{
	if(Children.size() == 1)
		return Children.back()->IsLeaf();

	return false;
}

//
// Pass on the emission call to all children of this node
//
// Note that this is called in various ways from different node types; see the other
// EmitFoo functions for details.
//
void DirectoryNode::EmitChildren(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->EmitAll(writer, virtualbaseaddress);
}

//
// Propagate an offset calculation to each child of this node
//
// Note that this is called in various ways from different node types; see the other
// SetOffsetsFoo functions for details.
//
void DirectoryNode::SetOffsetsChildren(DWORD& offset)
{
	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->SetOffsetsAll(offset);
}


//
// Construct and initialize a resource directory header structure
//
DirectoryHeader::DirectoryHeader(DWORD characteristics, DWORD timedatestamp, WORD majorversion, WORD minorversion)
	: Characteristics(characteristics),
	  TimeDateStamp(timedatestamp),
	  MajorVersion(majorversion),
	  MinorVersion(minorversion)
{
}

//
// Retrieve the on-disk size of a resource directory header structure
//
DWORD DirectoryHeader::GetSize() const
{
	return (sizeof(DWORD) * 2) + (sizeof(WORD) * 4);
}

//
// Write a resource directory structure to disk
//
void DirectoryHeader::EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	writer.EmitDWORD(Characteristics);
	writer.EmitDWORD(TimeDateStamp);
	writer.EmitWORD(MajorVersion);
	writer.EmitWORD(MinorVersion);
	writer.EmitWORD(0);
	writer.EmitWORD(static_cast<WORD>(Children.size()));
}

//
// Emit all data associated with a directory header node
//
// Note that because a directory header's child nodes are always directory entries, and that
// those nodes are stored in a flat array on disk, we must not simply recurse down the tree.
// Instead, we need to control the emission order as specified here to ensure all children's
// data is stored in the correct order on disk.
//
void DirectoryHeader::EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	EmitSelf(writer, virtualbaseaddress);

	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->EmitSelf(writer, virtualbaseaddress);

	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->EmitChildren(writer, virtualbaseaddress);
}

//
// Accumulate offsets for a directory header node
//
// Note that we only need to update the accumulator; since the header itself uses a flat
// array to store its children, there is no offset member field to adjust.
//
void DirectoryHeader::SetOffsetsSelf(DWORD& offset)
{
	offset += GetSize();
}

//
// Compute offsets for a directory header and its descendant nodes
//
// Similar to the emission process, we must do this in a careful order so that we preserve
// the semantics of a flat array for the direct children of the header node.
//
void DirectoryHeader::SetOffsetsAll(DWORD& offset)
{
	SetOffsetsSelf(offset);

	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		offset += (*iter)->GetSize();

	for(std::list<DirectoryNode*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
	{
		(*iter)->SetOffsetsSelf(offset);
		(*iter)->SetOffsetsChildren(offset);
	}
}

//
// Add a directory entry child under a directory header node
//
DirectoryEntry* DirectoryHeader::AddChild(DirectoryEntry* child)
{
	Children.push_back(child);
	return child;
}



//
// Construct and initialize a directory entry node
//
DirectoryEntry::DirectoryEntry(DWORD name, DWORD offsettodata)
	: Name(name),
	  OffsetToData(offsettodata)
{
}

//
// Retrieve the size that a directory entry requires on disk
//
DWORD DirectoryEntry::GetSize() const
{
	return sizeof(DWORD) * 2;
}

//
// Write a resource directory entry to disk
//
void DirectoryEntry::EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	writer.EmitDWORD(Name);
	writer.EmitDWORD(OffsetToData | (PointsToLeaf() ? 0 : 0x80000000));		// Set the high-order bit to flag a node entry (as opposed to a leaf entry)
}

void DirectoryEntry::EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	// Nothing to do, the header took care of writing us in the correct order
}

void DirectoryEntry::SetOffsetsSelf(DWORD& offset)
{
	OffsetToData = offset;
}

void DirectoryEntry::SetOffsetsAll(DWORD& offset)
{
	// Nothing to do
}

//
// Add a directory header child under a directory entry node
//
DirectoryHeader* DirectoryEntry::AddChild(DirectoryHeader* child)
{
	Children.push_back(child);
	return child;
}

//
// Add a directory leaf child under a directory entry node
//
DirectoryLeaf* DirectoryEntry::AddChild(DirectoryLeaf* child)
{
	Children.push_back(child);
	return child;
}


//
// Construct and initialize a resource leaf entry
//
// Leaf entries provide the actual offset and size of the
// given resource data, along with other useful info
//
DirectoryLeaf::DirectoryLeaf(DWORD offsettodata, DWORD size, DWORD codepage)
	: OffsetToData(offsettodata),
	  Size(size),
	  CodePage(codepage),
	  Reserved(0)
{
}

//
// Determine how much space the leaf entry requires on disk
//
DWORD DirectoryLeaf::GetSize() const
{
	return sizeof(DWORD) * 4;
}


//
// Write the leaf entry to disk
//
void DirectoryLeaf::EmitSelf(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	writer.EmitDWORD(OffsetToData + virtualbaseaddress);
	writer.EmitDWORD(Size);
	writer.EmitDWORD(CodePage);
	writer.EmitDWORD(Reserved);
}

void DirectoryLeaf::EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	EmitSelf(writer, virtualbaseaddress);
}

void DirectoryLeaf::SetOffsetsSelf(DWORD& offset)
{
	offset += GetSize();
}

void DirectoryLeaf::SetOffsetsAll(DWORD& offset)
{
	SetOffsetsSelf(offset);
}

void DirectoryLeaf::SetOffsetToData(DWORD offset)
{
	OffsetToData = offset;
}


//
// Construct and initialize the resource directory tracker
//
ResourceDirectory::ResourceDirectory()
	: RootTier(NULL),
	  ResourceSize(0),
	  DirectorySize(0),
	  OffsetsComputed(false)
{
}

//
// Destruct and clean up the directory data
//
ResourceDirectory::~ResourceDirectory()
{
	delete RootTier;

	for(std::list<ResourceRecord>::iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
		delete iter->Emitter;
}

//
// Add a resource to the directory
//
// Note that the emitter object is now owned by the directory
// manager and will be cleaned up when the manager is deleted.
//
void ResourceDirectory::AddResource(DWORD type, DWORD id, DWORD language, ResourceEmitter* emitter)
{
	ResourceRecords.push_back(ResourceRecord(type, id, language, emitter));
	OffsetsComputed = false;
}

//
// Traverse the directory and update each section's offsets
// to point to the correct locations
//
void ResourceDirectory::ComputeOffsets()
{
	std::map<const ResourceRecord*, DirectoryLeaf*> offsetmap;

	// First build the tree of resource directory nodes
	std::set<DWORD> resourcetypes;

	for(std::list<ResourceRecord>::const_iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
		resourcetypes.insert(iter->Type);

	delete RootTier;
	RootTier = new DirectoryHeader(0, 0, 0, 0);

	for(std::set<DWORD>::const_iterator typeiter = resourcetypes.begin(); typeiter != resourcetypes.end(); ++typeiter)
	{
		DWORD restype = *typeiter;
		
		DirectoryEntry* typeentry = RootTier->AddChild(new DirectoryEntry(restype, 0));
		DirectoryHeader* typeheader = typeentry->AddChild(new DirectoryHeader(0, 0, 0, 0));

		for(std::list<ResourceRecord>::const_iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
		{
			if(iter->Type == restype)
			{
				DWORD resid = iter->ID;

				DirectoryEntry* identry = typeheader->AddChild(new DirectoryEntry(resid, 0));
				DirectoryHeader* idheader = identry->AddChild(new DirectoryHeader(0, 0, 0, 0));
				
				for(std::list<ResourceRecord>::const_iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
				{
					if(iter->Type == restype && iter->ID == resid)
					{
						DirectoryEntry* langentry = idheader->AddChild(new DirectoryEntry(iter->Language, 0));
						offsetmap.insert(std::make_pair(&(*iter), langentry->AddChild(new DirectoryLeaf(0, iter->Emitter->GetSize(), 0))));
					}
				}
			}
		}
	}

	DWORD treeoffset = 0;
	RootTier->SetOffsetsAll(treeoffset);

	DWORD offset = 0;
	RootTier->AccumulateOffsets(offset);

	if(treeoffset != offset)
		throw Exception("Mismatched offsets during resource compilation");

	DirectorySize = offset;
	ResourceSize = 0;

	// Now compute resource data offsets
	for(std::list<ResourceRecord>::iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
	{
		// Update leaf entry with correct relative offset
		offsetmap.find(&(*iter))->second->SetOffsetToData(offset);
		iter->Emitter->SetOffset(offset);

		offset += iter->Emitter->GetSize();
		ResourceSize += iter->Emitter->GetSize();
	}

	OffsetsComputed = true;
}


//
// Determine how much disk space the resources will require
//
DWORD ResourceDirectory::GetSize() const
{
	if(!OffsetsComputed)
		throw Exception("Must call ComputeOffsets prior to retrieving size of resource directory");

	return ResourceSize + DirectorySize;
}

//
// Emit the resource directory and all resources to disk
//
void ResourceDirectory::Emit(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	if(!OffsetsComputed)
		throw Exception("Must call ComputeOffsets prior to emitting resource directory");

	RootTier->EmitAll(writer, virtualbaseaddress);

	for(std::list<ResourceRecord>::const_iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
		iter->Emitter->Emit(writer);
}


