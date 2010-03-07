//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for handling CUDA modules
//

#pragma once


// Dependencies
#include <cuda.h>

#include "Utility/Threading/Synchronization.h"


// Forward declarations
class FunctionCall;


class Module
{
// Construction and destruction
private:
	explicit Module(const std::string& modulefilename);
	Module(const std::vector<std::string>& functionnames, const void* codebuffer);
	~Module();

// Function access interface
public:
	FunctionCall CreateFunctionCall(const std::string& functionname);

// Module wrapper creation and management
public:
	static Module& LoadCUDAModule(const std::string& filename);
	static Module& LoadCUDAModule(const std::string& filename, const std::vector<std::string>& functionnames, const void* buffer);
	static void ReleaseAllModules();

// Compilation/serialization helpers
public:
	static std::string BuildSerializationData();

// Internal tracking
private:
	std::string FullFileName;
	CUmodule ModuleHandle;
	std::map<std::string, FunctionCall*> LoadedFunctions;
	Threads::CriticalSection CritSec;
};

