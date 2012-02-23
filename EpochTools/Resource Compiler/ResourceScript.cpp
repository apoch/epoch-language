//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper objects for managing resource script files
//

#include "pch.h"

#include "Resource Compiler/ResourceScript.h"
#include "Resource Compiler/ResourceDirectory.h"
#include "Resource Compiler/ResourceTypes.h"

#include "Resource Compiler/Resource Types/Icons.h"
#include "Resource Compiler/Resource Types/Menus.h"

#include "Utility/Files/FilesAndPaths.h"
#include "Utility/Strings.h"

#include <fstream>


using namespace ResourceCompiler;


//
// Construct and initialize a resource script wrapper
//
ResourceScript::ResourceScript(const std::list<std::wstring>& resourcefiles)
	: Filenames(resourcefiles)
{
	for(std::list<std::wstring>::const_iterator iter = Filenames.begin(); iter != Filenames.end(); ++iter)
		ProcessScriptFile(*iter);
}

//
// Destruct and clean up a resource script wrapper
//
ResourceScript::~ResourceScript()
{
	for(std::set<IconUnpacker*>::iterator iter = IconUnpackers.begin(); iter != IconUnpackers.end(); ++iter)
		delete *iter;
}


void ResourceScript::ProcessScriptFile(const std::wstring& filename)
{
	std::wifstream infile(filename.c_str());
	if(!infile)
		throw FileException("Failed to open resource script file");

	while(true)
	{
		std::wstring line;
		std::getline(infile, line);

		if(infile.eof())
			break;

		line = StripWhitespace(line);

		if(line.empty())
			continue;

		DWORD restype;
		if(line == L"[icon]")
			restype = RESTYPE_ICONGROUP;
		else if(line == L"[menu]")
			restype = RESTYPE_MENU;
		else
			throw Exception("Invalid directive in resource script");

		ResourceOffsets.insert(std::make_pair(restype, OffsetInfo(filename, infile.tellg())));

		do
		{
			std::getline(infile, line);
			line = StripWhitespace(line);
		} while(!line.empty() && !infile.eof());
	}
}


//
// Add the resources in the resource script to a resource directory manager
//
// The directory manager takes care of emitting all the resources and metadata
// to the final produced binary.
//
void ResourceScript::AddResourcesToDirectory(ResourceDirectory& directory)
{
	for(std::multimap<DWORD, OffsetInfo>::const_iterator iter = ResourceOffsets.begin(); iter != ResourceOffsets.end(); ++iter)
		LoadResourceIntoDirectory(iter->first, iter->second.Filename, iter->second.Offset, directory);
}

//
// Read resource information out of the script file and add a corresponding resource to the directory
//
void ResourceScript::LoadResourceIntoDirectory(DWORD type, const std::wstring& filename, size_t offset, ResourceDirectory& directory)
{
	DWORD id, language;
	std::auto_ptr<ResourceEmitter> emitter(NULL);

	std::wifstream infile(filename.c_str());
	if(!infile)
		throw FileException("Failed to open resource script file");

	infile.seekg(static_cast<std::streamoff>(offset));

	std::wstring directive;

	infile >> directive;
	if(StripWhitespace(directive) != L"id")
		throw Exception("Expected resource ID");
	infile >> id;
	
	infile >> directive;
	if(StripWhitespace(directive) != L"language")
		throw Exception("Expected resource language");
	infile >> language;

	switch(type)
	{
	case RESTYPE_ICONGROUP:
		{
			std::wstring sourcefile;

			infile >> directive;
			if(StripWhitespace(directive) != L"source")
				throw Exception("Expected icon source file");
			infile.ignore();
			std::getline(infile, sourcefile);

			sourcefile = StripFilename(filename) + StripQuotes(StripWhitespace(sourcefile));

			std::auto_ptr<IconUnpacker> unpacker(new IconUnpacker(sourcefile, id, language));
			unpacker->CreateResources(directory, GroupMemberships);
			IconUnpackers.insert(unpacker.release());

			emitter.reset(new IconGroupEmitter(id, GroupMemberships));
		}
		break;

	case RESTYPE_MENU:
		emitter.reset(new MenuEmitter(infile));
		break;

	default:
		throw Exception("Unrecognized resource type; cannot load from script");
	}

	if(!emitter.get() || !id || !language)
		throw Exception("Failed to read resource from script file");

	directory.AddResource(type, id, language, emitter.release());
}


