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
// Construct and initialize a resource directory header structure
//
DirectoryHeader::DirectoryHeader(DWORD characteristics, DWORD timedatestamp, WORD majorversion, WORD minorversion, WORD numnamedentries, WORD numidentries)
	: Characteristics(characteristics),
	  TimeDateStamp(timedatestamp),
	  MajorVersion(majorversion),
	  MinorVersion(minorversion),
	  NumNamedEntries(numnamedentries),
	  NumIDEntries(numidentries)
{
}

//
// Retrieve the size of a resource directory header structure
//
DWORD DirectoryHeader::GetSize() const
{
	return GetSizeStatic();
}

DWORD DirectoryHeader::GetSizeStatic()
{
	return (sizeof(DWORD) * 2) + (sizeof(WORD) * 4);
}

//
// Write a resource directory structure to disk
//
void DirectoryHeader::Emit(LinkWriter& writer, bool pointstoleaf, DWORD virtualbaseaddress)
{
	writer.EmitDWORD(Characteristics);
	writer.EmitDWORD(TimeDateStamp);
	writer.EmitWORD(MajorVersion);
	writer.EmitWORD(MinorVersion);
	writer.EmitWORD(NumNamedEntries);
	writer.EmitWORD(NumIDEntries);
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
	return GetSizeStatic();
}

DWORD DirectoryEntry::GetSizeStatic()
{
	return sizeof(DWORD) * 2;
}

//
// Write a resource directory entry to disk
//
void DirectoryEntry::Emit(LinkWriter& writer, bool pointstoleaf, DWORD virtualbaseaddress)
{
	writer.EmitDWORD(Name);
	writer.EmitDWORD(OffsetToData | (pointstoleaf ? 0 : 0x80000000));		// Set the high-order bit to flag a node entry (as opposed to a leaf entry)
}

//
// Construct and initialize a resource leaf entry
//
// Leaf entries provide the actual offset and size of the
// given resource data, along with other useful info
//
DirectoryLeaf::DirectoryLeaf(DWORD offsettodata, DWORD size, DWORD codepage, DWORD boundresid)
	: OffsetToData(offsettodata),
	  Size(size),
	  CodePage(codepage),
	  Reserved(0),
	  BoundResourceID(boundresid)
{
}

//
// Determine how much space the leaf entry requires on disk
//
DWORD DirectoryLeaf::GetSize() const
{
	return GetSizeStatic();
}

DWORD DirectoryLeaf::GetSizeStatic()
{
	return sizeof(DWORD) * 4;
}


//
// Write the leaf entry to disk
//
void DirectoryLeaf::Emit(LinkWriter& writer, bool pointstoleaf, DWORD virtualbaseaddress)
{
	writer.EmitDWORD(OffsetToData + virtualbaseaddress);
	writer.EmitDWORD(Size);
	writer.EmitDWORD(CodePage);
	writer.EmitDWORD(Reserved);
}



//
// Construct and initialize the resource directory tracker
//
ResourceDirectory::ResourceDirectory()
{
	std::auto_ptr<DirectoryHeader> ptr(new DirectoryHeader(0, 0, 0, 0, 0, 0));
	RootTierHeader = ptr.get();
	RootTier.push_back(ptr.release());
}

//
// Destruct and clean up the directory data
//
ResourceDirectory::~ResourceDirectory()
{
	for(std::list<DirectoryBase*>::iterator iter = RootTier.begin(); iter != RootTier.end(); ++iter)
		delete *iter;

	for(std::list<DirectoryBase*>::iterator iter = IDTier.begin(); iter != IDTier.end(); ++iter)
		delete *iter;

	for(std::list<DirectoryBase*>::iterator iter = LanguageTier.begin(); iter != LanguageTier.end(); ++iter)
		delete *iter;

	for(std::list<DirectoryBase*>::iterator iter = LeafTier.begin(); iter != LeafTier.end(); ++iter)
		delete *iter;

	for(std::map<DWORD, ResourceEmitter*>::iterator iter = ResourceEmitters.begin(); iter != ResourceEmitters.end(); ++iter)
		delete iter->second;
}

//
// Add a resource to the directory
//
// Note that the emitter object is now owned by the directory
// manager and will be cleaned up when the manager is deleted.
//
void ResourceDirectory::AddResource(DWORD type, DWORD id, DWORD language, ResourceEmitter* emitter)
{
	if(ResourceEmitters.find(id) != ResourceEmitters.end())
	{
		std::ostringstream msg;
		msg << "Resource ID " << id << " is already in use!";
		throw Exception(msg.str());
	}

	ResourceEmitters[id] = emitter;

	++RootTierHeader->NumIDEntries;
	RootTier.push_back(new DirectoryEntry(type, 0));

	IDTier.push_back(new DirectoryHeader(0, 0, 0, 0, 0, 1));
	IDTier.push_back(new DirectoryEntry(id, 0));

	LanguageTier.push_back(new DirectoryHeader(0, 0, 0, 0, 0, 1));
	LanguageTier.push_back(new DirectoryEntry(language, 0));

	LeafTier.push_back(new DirectoryLeaf(0, emitter->GetSize(), 0, id));
}

//
// Traverse the directory and update each section's offsets
// to point to the correct locations
//
void ResourceDirectory::ComputeOffsets()
{
	DWORD offset = 0;

	// Compute the offset of the end of the root block
	offset = DirectoryHeader::GetSizeStatic() + DirectoryEntry::GetSizeStatic() * (RootTierHeader->NumIDEntries + RootTierHeader->NumNamedEntries);

	// Populate offset fields of records in the root tier
	for(std::list<DirectoryBase*>::const_iterator iter = RootTier.begin(); iter != RootTier.end(); ++iter)
	{
		if((*iter)->SetOffset(offset))
			offset += DirectoryHeader::GetSizeStatic() + DirectoryEntry::GetSizeStatic();
	}

	// Populate offset fields of records in the ID tier
	for(std::list<DirectoryBase*>::const_iterator iter = IDTier.begin(); iter != IDTier.end(); ++iter)
	{
		if((*iter)->SetOffset(offset))
			offset += DirectoryHeader::GetSizeStatic() + DirectoryEntry::GetSizeStatic();
	}

	// Populate offset fields of records in the language tier
	for(std::list<DirectoryBase*>::const_iterator iter = LanguageTier.begin(); iter != LanguageTier.end(); ++iter)
	{
		if((*iter)->SetOffset(offset))
			offset += DirectoryLeaf::GetSizeStatic();
	}

	DirectorySize = offset;
	ResourceSize = 0;

	// Now compute resource data offsets
	for(std::map<DWORD, ResourceEmitter*>::iterator iter = ResourceEmitters.begin(); iter != ResourceEmitters.end(); ++iter)
	{
		// Update leaf entry with correct relative offset
		for(std::list<DirectoryBase*>::const_iterator leafiter = LeafTier.begin(); leafiter != LeafTier.end(); ++leafiter)
		{
			DirectoryLeaf* leaf = dynamic_cast<DirectoryLeaf*>(*leafiter);
			if(leaf == NULL)
				throw Exception("Resource directory leaf list contains a non-leaf node!");

			if(leaf->BoundResourceID == iter->first)
			{
				leaf->OffsetToData = offset;
				break;
			}
		}

		iter->second->SetOffset(offset);
		offset += iter->second->GetSize();
		ResourceSize += iter->second->GetSize();
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
void ResourceDirectory::Emit(LinkWriter& writer, DWORD virtualbaseaddress)
{
	for(std::list<DirectoryBase*>::const_iterator iter = RootTier.begin(); iter != RootTier.end(); ++iter)
		(*iter)->Emit(writer, false, virtualbaseaddress);

	for(std::list<DirectoryBase*>::const_iterator iter = IDTier.begin(); iter != IDTier.end(); ++iter)
		(*iter)->Emit(writer, false, virtualbaseaddress);

	for(std::list<DirectoryBase*>::const_iterator iter = LanguageTier.begin(); iter != LanguageTier.end(); ++iter)
		(*iter)->Emit(writer, true, virtualbaseaddress);

	for(std::list<DirectoryBase*>::const_iterator iter = LeafTier.begin(); iter != LeafTier.end(); ++iter)
		(*iter)->Emit(writer, false, virtualbaseaddress);

	for(std::map<DWORD, ResourceEmitter*>::const_iterator iter = ResourceEmitters.begin(); iter != ResourceEmitters.end(); ++iter)
		iter->second->Emit(writer);
}

