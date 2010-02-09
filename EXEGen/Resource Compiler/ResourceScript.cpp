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


using namespace ResourceCompiler;


// Prototypes
std::wstring StripQuotes(const std::wstring& str);


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
	for(std::multimap<DWORD, IconEmitter*>::iterator iter = IconEmitters.begin(); iter != IconEmitters.end(); ++iter)
		delete iter->second;
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

		if(line.empty())
			continue;

		DWORD restype;
		if(line == L"[icon]")
			restype = RESTYPE_ICON;
		else if(line == L"[menu]")
			restype = RESTYPE_MENU;
		else if(line == L"[icongroup]")
			restype = RESTYPE_ICONGROUP;
		else
			throw Exception("Invalid directive in resource script");

		ResourceOffsets.insert(std::make_pair(restype, OffsetInfo(filename, infile.tellg())));

		do
		{
			std::getline(infile, line);
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
	if(directive != L"id")
		throw Exception("Expected resource ID");
	infile >> id;
	
	infile >> directive;
	if(directive != L"language")
		throw Exception("Expected resource language");
	infile >> language;

	switch(type)
	{
	case RESTYPE_ICON:
		{
			DWORD group;
			std::wstring sourcefile;

			infile >> directive;
			if(directive != L"group")
				throw Exception("Expected icon group ID");
			infile >> group;

			infile >> directive;
			if(directive != L"source")
				throw Exception("Expected icon source file");
			infile.ignore();
			std::getline(infile, sourcefile);

			sourcefile = StripQuotes(sourcefile);

			emitter.reset(new IconEmitter(sourcefile));
		}
		break;

	case RESTYPE_ICONGROUP:
		emitter.reset(new IconGroupEmitter(id, IconEmitters));
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


//
// Helper for removing quotes from around a string
//
std::wstring StripQuotes(const std::wstring& str)
{
	std::wstring ret(str);

	if(ret.empty())
		return ret;

	if(ret[0] == L'\"')
		ret = ret.substr(1);

	if(*ret.rbegin() == L'\"')
		ret = ret.substr(0, ret.length() - 1);

	return ret;
}

