//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Helper routines for generating identifier names
//

#include "pch.h"

#include "CUDA Wrapper/Naming.h"



//
// Given a code block handle, create a unique function name for the corresponding code
//
// This is done to ensure that various CUDA entry points are all available in the final
// JIT-compiled code; it also provides a reliable way to invoke specific bits of CUDA.
//
std::string GenerateFunctionName(Extensions::OriginalCodeHandle handle)
{
	std::ostringstream stream;
	stream << "cuda_interop_" << handle;
	return stream.str();
}

