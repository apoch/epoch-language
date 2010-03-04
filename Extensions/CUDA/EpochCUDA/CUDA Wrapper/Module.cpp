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

	if(cuModuleLoad(&ModuleHandle, modulefilename.c_str()) != CUDA_SUCCESS)
		throw std::exception("Failed to load CUDA assembly module");
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

	std::map<std::string, Module*>::const_iterator iter = LoadedModules.find(filename);
	if(iter != LoadedModules.end())
		return *(iter->second);

	return *(LoadedModules.insert(std::make_pair(filename, new Module(filename))).first->second);
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

