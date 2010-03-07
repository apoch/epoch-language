//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for handling CUDA modules
//

#include "pch.h"

#include "CUDA Wrapper/Module.h"
#include "CUDA Wrapper/FunctionCall.h"

#include "Utility/Threading/Synchronization.h"
#include "Utility/Strings.h"
#include "Utility/Files/FilesAndPaths.h"
#include "Utility/Files/Files.h"


// TODO - centralize the declarations of these variables
extern bool CUDAAvailableForExecution;
extern bool CUDALibraryLoaded;


namespace
{
	// Internal tracking of loaded CUDA modules; used to avoid loading modules more than once
	std::map<std::string, Module*> LoadedModules;

	Threads::CriticalSection ModuleListCriticalSection;
}


//
// Construct and initialize a module wrapper (loads the module file from disk)
//
Module::Module(const std::string& modulefilename)
{
	if(!CUDAAvailableForExecution)
		return;

	FullFileName = modulefilename;

	HANDLE FileHandle = ::CreateFile(widen(modulefilename).c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(FileHandle == INVALID_HANDLE_VALUE)
		throw std::exception("Failed to load CUDA assembly module - file not found");

	HANDLE Mapping = ::CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	if(!Mapping)
	{
		::CloseHandle(FileHandle);
		throw std::exception("Cannot map CUDA file for execution");
	}

	void* EntireFileBuffer = ::MapViewOfFile(Mapping, FILE_MAP_READ, 0, 0, 0);

	if(cuModuleLoadData(&ModuleHandle, EntireFileBuffer) != CUDA_SUCCESS)
		throw std::exception("Failed to load CUDA assembly module");

	::UnmapViewOfFile(EntireFileBuffer);
	::CloseHandle(Mapping);
	::CloseHandle(FileHandle);
}

Module::Module(const std::vector<std::string>& functionnames, const void* codebuffer)
{
	if(!CUDAAvailableForExecution)
		return;

	if(cuModuleLoadData(&ModuleHandle, codebuffer) != CUDA_SUCCESS)
		throw std::exception("Failed to load CUDA assembly module");

	for(std::vector<std::string>::const_iterator iter = functionnames.begin(); iter != functionnames.end(); ++iter)
		CreateFunctionCall(*iter);
}


//
// Destruct and clean up a module wrapper
//
Module::~Module()
{
	for(std::map<std::string, FunctionCall*>::iterator iter = LoadedFunctions.begin(); iter != LoadedFunctions.end(); ++iter)
		delete iter->second;

	if(CUDAAvailableForExecution)
		cuModuleUnload(ModuleHandle);
}


//
// Create a wrapper object that can be used for calling functions in this module
//
FunctionCall Module::CreateFunctionCall(const std::string& functionname)
{
	if(!CUDAAvailableForExecution)
		return FunctionCall(0);

	Threads::CriticalSection::Auto mutex(CritSec);

	std::map<std::string, FunctionCall*>::const_iterator iter = LoadedFunctions.find(functionname);
	if(iter != LoadedFunctions.end())
		return *(iter->second);

	CUfunction functionhandle;
	CUresult result = cuModuleGetFunction(&functionhandle, ModuleHandle, functionname.c_str());
	if(result != CUDA_SUCCESS)
		throw std::exception("Failed to locate the requested CUDA interop function");

	return *(LoadedFunctions.insert(std::make_pair(functionname, new FunctionCall(functionhandle))).first->second);
}


//
// Static helper: retrieve (or load) the requested module, given a module filename
//
Module& Module::LoadCUDAModule(const std::string& filename)
{
	Threads::CriticalSection::Auto mutex(ModuleListCriticalSection);

	std::string filenopath = narrow(StripPath(widen(filename)));
	std::map<std::string, Module*>::const_iterator iter = LoadedModules.find(filenopath);
	if(iter != LoadedModules.end())
		return *(iter->second);

	return *(LoadedModules.insert(std::make_pair(filenopath, new Module(filename))).first->second);
}

Module& Module::LoadCUDAModule(const std::string& filename, const std::vector<std::string>& functionnames, const void* buffer)
{
	Threads::CriticalSection::Auto mutex(ModuleListCriticalSection);

	std::map<std::string, Module*>::const_iterator iter = LoadedModules.find(filename);
	if(iter != LoadedModules.end())
		return *(iter->second);

	return *(LoadedModules.insert(std::make_pair(filename, new Module(functionnames, buffer))).first->second);
}


//
// Static helper: free all loaded modules
//
void Module::ReleaseAllModules()
{
	Threads::CriticalSection::Auto mutex(ModuleListCriticalSection);

	for(std::map<std::string, Module*>::iterator iter = LoadedModules.begin(); iter != LoadedModules.end(); ++iter)
		delete iter->second;
}


std::string Module::BuildSerializationData()
{
	Threads::CriticalSection::Auto mutex(ModuleListCriticalSection);

	std::stringstream stream;

	stream << LoadedModules.size() << "\n";
	for(std::map<std::string, Module*>::const_iterator iter = LoadedModules.begin(); iter != LoadedModules.end(); ++iter)
	{
		stream << iter->first << " " << iter->second->LoadedFunctions.size() << "\n";
		for(std::map<std::string, FunctionCall*>::const_iterator funciter = iter->second->LoadedFunctions.begin(); funciter != iter->second->LoadedFunctions.end(); ++funciter)
			stream << funciter->first << "\n";

		std::vector<Byte> temp;
		stream << Files::GetFileSize(iter->second->FullFileName.c_str()) << "\n";
		Files::Load(iter->second->FullFileName.c_str(), temp);

		stream.write(&temp[0], static_cast<std::streamsize>(temp.size()));
		stream << "\n";
	}

	return stream.str();
}

