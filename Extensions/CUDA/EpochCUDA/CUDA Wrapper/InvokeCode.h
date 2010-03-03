//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Wrapper class for invoking generated CUDA code
//

#pragma once


// Dependencies
#include <cuda.h>

#include "Traverser/TraversalInterface.h"
#include "Language Extensions/HandleTypes.h"


//
// Wrapper class for invoking CUDA code
//
class CUDACodeInvoker
{
// Construction and destruction
public:
	CUDACodeInvoker(Extensions::CodeBlockHandle codehandle, HandleType activatedscopehandle);

// CUDA execution interface
public:
	void Execute(size_t lowerbound, size_t upperbound);

// Internal tracking
private:
	Extensions::CodeBlockHandle CodeHandle;
	HandleType ActivatedScopeHandle;

	std::string FunctionName;

	const std::list<Traverser::ScopeContents>& RegisteredVariables;
};

