//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for handling CUDA modules
//

#pragma once


// Dependencies
#include <cuda.h>


// Forward declarations
class FunctionCall;


class Module
{
// Construction and destruction
private:
	Module(const std::string& modulefilename);
	~Module();

// Function access interface
public:
	FunctionCall CreateFunctionCall(const std::string& functionname);

// Module wrapper creation and management
public:
	static Module& LoadCUDAModule(const std::string& filename);
	static void ReleaseAllModules();

// Internal tracking
private:
	CUmodule ModuleHandle;
	std::map<std::string, FunctionCall*> LoadedFunctions;
};

