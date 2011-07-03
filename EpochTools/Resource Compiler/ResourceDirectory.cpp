//
// The Epoch Language Project
// Win32 EXE Generator
//
// Helper interfaces for the resource manager
//

#include "pch.h"

#include "Resource Compiler/ResourceDirectory.h"
#include "Resource Compiler/ResourceEmitter.h"

#include "Linker/LinkWriter.h"


using namespace ResourceCompiler;

//
// Accumulate the total offset from the beginning of a directory node to the end of all of its descendants
//
void DirectoryBase::AccumulateOffsets(DWORD& offset)
{
	offset += GetSize();
	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->AccumulateOffsets(offset);
}

bool DirectoryBase::PointsToLeaf() const
{
	if(Children.empty())
		return false;

	if(Children.size() == 1)
	{
		DirectoryBase* b = Children.back();
		return (dynamic_cast<DirectoryLeaf*>(b) != NULL);
	}

	return false;
}

//
// Build a list of each leaf node in the tree
//
void DirectoryBase::AccumulateLeaves(std::list<DirectoryLeaf*>& leaves) const
{
	if(dynamic_cast<const DirectoryLeaf*>(this))
		leaves.push_back(const_cast<DirectoryLeaf*>(dynamic_cast<const DirectoryLeaf*>(this)));

	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->AccumulateLeaves(leaves);
}


void DirectoryBase::EmitChildren(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->EmitAll(writer, virtualbaseaddress);
}

void DirectoryBase::SetOffsetsChildren(DWORD& offset)
{
	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
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
// Retrieve the size of a resource directory header structure
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

void DirectoryHeader::EmitAll(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	EmitSelf(writer, virtualbaseaddress);

	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->EmitSelf(writer, virtualbaseaddress);

	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		(*iter)->EmitChildren(writer, virtualbaseaddress);
}

void DirectoryHeader::SetOffsetsSelf(DWORD& offset)
{
	offset += GetSize();
}

void DirectoryHeader::SetOffsetsAll(DWORD& offset)
{
	SetOffsetsSelf(offset);

	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
		offset += (*iter)->GetSize();

	for(std::list<DirectoryBase*>::const_iterator iter = Children.begin(); iter != Children.end(); ++iter)
	{
		(*iter)->SetOffsetsSelf(offset);
		(*iter)->SetOffsetsChildren(offset);
	}
}


//
// Construct and initialize a directory entry structure
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

//
// Construct and initialize the resource directory tracker
//
ResourceDirectory::ResourceDirectory()
	: RootTier(NULL),
	  ResourceSize(0),
	  DirectorySize(0)
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

		RootTier->Children.push_back(new DirectoryEntry(restype, 0));
		DirectoryBase* typeentry = RootTier->Children.back();
		typeentry->Children.push_back(new DirectoryHeader(0, 0, 0, 0));
		DirectoryBase* typeheader = typeentry->Children.back();

		for(std::list<ResourceRecord>::const_iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
		{
			if(iter->Type == restype)
			{
				DWORD resid = iter->ID;

				typeheader->Children.push_back(new DirectoryEntry(resid, 0));
				DirectoryBase* identry = typeheader->Children.back();
				identry->Children.push_back(new DirectoryHeader(0, 0, 0, 0));
				DirectoryBase* idheader = identry->Children.back();
				
				for(std::list<ResourceRecord>::const_iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
				{
					if(iter->Type == restype && iter->ID == resid)
					{
						idheader->Children.push_back(new DirectoryEntry(iter->Language, 0));
						DirectoryBase* langentry = idheader->Children.back();
						langentry->Children.push_back(new DirectoryLeaf(0, iter->Emitter->GetSize(), 0));

						offsetmap.insert(std::make_pair(&(*iter), static_cast<DirectoryLeaf*>(langentry->Children.back())));
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
		offsetmap.find(&(*iter))->second->OffsetToData = offset;

		iter->Emitter->SetOffset(offset);
		offset += iter->Emitter->GetSize();
		ResourceSize += iter->Emitter->GetSize();
	}
}


//
// Determine how much disk space the resources will require
//
DWORD ResourceDirectory::GetSize() const
{
	return ResourceSize + DirectorySize;
}

//
// Emit the resource directory and all resources to disk
//
void ResourceDirectory::Emit(LinkWriter& writer, DWORD virtualbaseaddress) const
{
	RootTier->EmitAll(writer, virtualbaseaddress);

	for(std::list<ResourceRecord>::const_iterator iter = ResourceRecords.begin(); iter != ResourceRecords.end(); ++iter)
		iter->Emitter->Emit(writer);
}

